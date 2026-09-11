/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Extracted from libcamera v0.7.2 IPU3 imgu.cpp. Copyright (C) 2019, Google Inc. */
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <libcamera/geometry.h>
using namespace libcamera;
namespace utils {
constexpr unsigned int alignUp(unsigned int v, unsigned int a) { return (v+a-1)/a*a; }
template<class T> auto abs_diff(T a,T b) { return a>b?a-b:b-a; }
}
struct NullLog { template<class T> NullLog& operator<<(const T&) {return *this;} };
#define LOG(a,b) NullLog()
class ImgUDevice
{
public:
	static constexpr unsigned int kFilterWidth = 4;
	static constexpr unsigned int kFilterHeight = 4;

	static constexpr unsigned int kIFAlignWidth = 2;
	static constexpr unsigned int kIFAlignHeight = 4;

	static constexpr unsigned int kIFMaxCropWidth = 40;
	static constexpr unsigned int kIFMaxCropHeight = 540;

	static constexpr unsigned int kBDSAlignWidth = 2;
	static constexpr unsigned int kBDSAlignHeight = 4;

	static constexpr float kBDSSfMax = 2.5;
	static constexpr float kBDSSfMin = 1.0;
	static constexpr float kBDSSfStep = 0.03125;

	static constexpr Size kOutputMinSize = { 2, 2 };
	static constexpr Size kOutputMaxSize = { 4480, 34004 };
	static constexpr unsigned int kOutputAlignWidth = 64;
	static constexpr unsigned int kOutputAlignHeight = 4;
	static constexpr unsigned int kOutputMarginWidth = 64;
	static constexpr unsigned int kOutputMarginHeight = 32;

	struct PipeConfig {
		float bds_sf;
		Size iif;
		Size bds;
		Size gdc;

		bool isNull() const
		{
			return iif.isNull() || bds.isNull() || gdc.isNull();
		}
	};

	struct Pipe {
		Size input;
		Size main;
		Size viewfinder;
	};

 PipeConfig calculatePipeConfig(Pipe *pipe);
};
namespace {

/*
 * The procedure to calculate the ImgU pipe configuration has been ported
 * from the pipe_config.py python script, available at:
 * https://github.com/intel/intel-ipu3-pipecfg
 * at revision: 243d13446e44 ("Fix some bug for some resolutions")
 */

/* BSD scaling factors: min=1, max=2.5, step=1/32 */
const std::vector<float> bdsScalingFactors = {
	1, 1.03125, 1.0625, 1.09375, 1.125, 1.15625, 1.1875, 1.21875, 1.25,
	1.28125, 1.3125, 1.34375, 1.375, 1.40625, 1.4375, 1.46875, 1.5, 1.53125,
	1.5625, 1.59375, 1.625, 1.65625, 1.6875, 1.71875, 1.75, 1.78125, 1.8125,
	1.84375, 1.875, 1.90625, 1.9375, 1.96875, 2, 2.03125, 2.0625, 2.09375,
	2.125, 2.15625, 2.1875, 2.21875, 2.25, 2.28125, 2.3125, 2.34375, 2.375,
	2.40625, 2.4375, 2.46875, 2.5
};

/* GDC scaling factors: min=1, max=16, step=1/4 */
const std::vector<float> gdcScalingFactors = {
	1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 2.75, 3, 3.25, 3.5, 3.75, 4, 4.25,
	4.5, 4.75, 5, 5.25, 5.5, 5.75, 6, 6.25, 6.5, 6.75, 7, 7.25, 7.5, 7.75,
	8, 8.25, 8.5, 8.75, 9, 9.25, 9.5, 9.75, 10, 10.25, 10.5, 10.75, 11,
	11.25, 11.5, 11.75, 12, 12.25, 12.5, 12.75, 13, 13.25, 13.5, 13.75, 14,
	14.25, 14.5, 14.75, 15, 15.25, 15.5, 15.75, 16,
};

std::vector<ImgUDevice::PipeConfig> pipeConfigs;

struct FOV {
	float w;
	float h;

	bool isLarger(const FOV &other)
	{
		if (w > other.w)
			return true;
		if (w == other.w && h > other.h)
			return true;
		return false;
	}
};

/* Approximate a scaling factor sf to the closest one available in a range. */
float findScaleFactor(float sf, const std::vector<float> &range,
		      bool roundDown = false)
{
	if (sf <= range[0])
		return range[0];
	if (sf >= range[range.size() - 1])
		return range[range.size() - 1];

	float bestDiff = std::numeric_limits<float>::max();
	unsigned int index = 0;
	for (unsigned int i = 0; i < range.size(); ++i) {
		float diff = utils::abs_diff(sf, range[i]);
		if (diff < bestDiff) {
			bestDiff = diff;
			index = i;
		}
	}

	if (roundDown && index > 0 && sf < range[index])
		index--;

	return range[index];
}

bool isSameRatio(const Size &in, const Size &out)
{
	float inRatio = static_cast<float>(in.width) / in.height;
	float outRatio = static_cast<float>(out.width) / out.height;

	if (utils::abs_diff(inRatio, outRatio) > 0.1)
		return false;

	return true;
}

void calculateBDSHeight(ImgUDevice::Pipe *pipe, const Size &iif, const Size &gdc,
			unsigned int bdsWidth, float bdsSF)
{
	/* Keep crop bounds ordered, including for small intermediate heights. */
	unsigned int minIFHeight = iif.height > ImgUDevice::kIFMaxCropHeight
		? iif.height - ImgUDevice::kIFMaxCropHeight : 1;
	unsigned int minBDSHeight = gdc.height + ImgUDevice::kFilterHeight * 2;
	unsigned int ifHeight;
	float bdsHeight;

	if (!isSameRatio(pipe->input, gdc)) {
		unsigned int foundIfHeight = 0;
		float estIFHeight = (iif.width * gdc.height) /
				    static_cast<float>(gdc.width);
		estIFHeight = std::clamp<float>(estIFHeight, minIFHeight, iif.height);

		ifHeight = utils::alignUp(estIFHeight, ImgUDevice::kIFAlignHeight);
		while (ifHeight >= minIFHeight && ifHeight <= iif.height &&
		       ifHeight / bdsSF >= minBDSHeight) {

			float height = ifHeight / bdsSF;
			if (std::fmod(height, 1.0) == 0) {
				unsigned int bdsIntHeight = static_cast<unsigned int>(height);

				if (!(bdsIntHeight % ImgUDevice::kBDSAlignHeight)) {
					foundIfHeight = ifHeight;
					bdsHeight = height;
					break;
				}
			}

			ifHeight -= ImgUDevice::kIFAlignHeight;
		}

		ifHeight = utils::alignUp(estIFHeight, ImgUDevice::kIFAlignHeight);
		while (ifHeight >= minIFHeight && ifHeight <= iif.height &&
		       ifHeight / bdsSF >= minBDSHeight) {

			float height = ifHeight / bdsSF;
			if (std::fmod(height, 1.0) == 0) {
				unsigned int bdsIntHeight = static_cast<unsigned int>(height);

				if (!(bdsIntHeight % ImgUDevice::kBDSAlignHeight)) {
					foundIfHeight = ifHeight;
					bdsHeight = height;
					break;
				}
			}

			ifHeight += ImgUDevice::kIFAlignHeight;
		}

		if (foundIfHeight) {
			unsigned int bdsIntHeight = static_cast<unsigned int>(bdsHeight);

			pipeConfigs.push_back({ bdsSF, { iif.width, foundIfHeight },
						{ bdsWidth, bdsIntHeight }, gdc });
			return;
		}
	} else {
		ifHeight = utils::alignUp(iif.height, ImgUDevice::kIFAlignHeight);
		while (ifHeight >= minIFHeight && ifHeight / bdsSF >= minBDSHeight) {

			bdsHeight = ifHeight / bdsSF;
			if (std::fmod(ifHeight, 1.0) == 0 && std::fmod(bdsHeight, 1.0) == 0) {
				unsigned int bdsIntHeight = static_cast<unsigned int>(bdsHeight);

				if (!(ifHeight % ImgUDevice::kIFAlignHeight) &&
				    !(bdsIntHeight % ImgUDevice::kBDSAlignHeight)) {
					pipeConfigs.push_back({ bdsSF, { iif.width, ifHeight },
								{ bdsWidth, bdsIntHeight }, gdc });
				}
			}

			ifHeight -= ImgUDevice::kIFAlignHeight;
		}
	}
}

void calculateBDS(ImgUDevice::Pipe *pipe, const Size &iif, const Size &gdc, float bdsSF)
{
	unsigned int minBDSWidth = gdc.width + ImgUDevice::kFilterWidth * 2;
	unsigned int minBDSHeight = gdc.height + ImgUDevice::kFilterHeight * 2;

	float sf = bdsSF;
	while (sf <= ImgUDevice::kBDSSfMax && sf >= ImgUDevice::kBDSSfMin) {
		float bdsWidth = static_cast<float>(iif.width) / sf;
		float bdsHeight = static_cast<float>(iif.height) / sf;

		if (std::fmod(bdsWidth, 1.0) == 0 &&
		    std::fmod(bdsHeight, 1.0) == 0) {
			unsigned int bdsIntWidth = static_cast<unsigned int>(bdsWidth);
			unsigned int bdsIntHeight = static_cast<unsigned int>(bdsHeight);
			if (!(bdsIntWidth % ImgUDevice::kBDSAlignWidth) && bdsWidth >= minBDSWidth &&
			    !(bdsIntHeight % ImgUDevice::kBDSAlignHeight) && bdsHeight >= minBDSHeight)
				calculateBDSHeight(pipe, iif, gdc, bdsIntWidth, sf);
		}

		sf += ImgUDevice::kBDSSfStep;
	}

	sf = bdsSF;
	while (sf <= ImgUDevice::kBDSSfMax && sf >= ImgUDevice::kBDSSfMin) {
		float bdsWidth = static_cast<float>(iif.width) / sf;
		float bdsHeight = static_cast<float>(iif.height) / sf;

		if (std::fmod(bdsWidth, 1.0) == 0 &&
		    std::fmod(bdsHeight, 1.0) == 0) {
			unsigned int bdsIntWidth = static_cast<unsigned int>(bdsWidth);
			unsigned int bdsIntHeight = static_cast<unsigned int>(bdsHeight);
			if (!(bdsIntWidth % ImgUDevice::kBDSAlignWidth) && bdsWidth >= minBDSWidth &&
			    !(bdsIntHeight % ImgUDevice::kBDSAlignHeight) && bdsHeight >= minBDSHeight)
				calculateBDSHeight(pipe, iif, gdc, bdsIntWidth, sf);
		}

		sf -= ImgUDevice::kBDSSfStep;
	}
}

Size calculateGDC(ImgUDevice::Pipe *pipe)
{
	const Size &in = pipe->input;
	const Size &main = pipe->main;
	const Size &vf = pipe->viewfinder;
	Size gdc;

	if (!vf.isNull()) {
		gdc.width = main.width;

		float ratio = (main.width * vf.height) / static_cast<float>(vf.width);
		gdc.height = std::max(static_cast<float>(main.height), ratio);

		return gdc;
	}

	if (!isSameRatio(in, main)) {
		gdc = main;
		return gdc;
	}

	float totalSF = static_cast<float>(in.width) / main.width;
	float bdsSF = totalSF > 2 ? 2 : 1;
	float yuvSF = totalSF / bdsSF;
	float sf = findScaleFactor(yuvSF, gdcScalingFactors);

	gdc.width = main.width * sf;
	gdc.height = main.height * sf;

	return gdc;
}

FOV calcFOV(const Size &in, const ImgUDevice::PipeConfig &pipe)
{
	FOV fov{};

	float inW = static_cast<float>(in.width);
	float inH = static_cast<float>(in.height);
	float ifCropW = static_cast<float>(in.width - pipe.iif.width);
	float ifCropH = static_cast<float>(in.height - pipe.iif.height);
	float gdcCropW = static_cast<float>(pipe.bds.width - pipe.gdc.width) * pipe.bds_sf;
	float gdcCropH = static_cast<float>(pipe.bds.height - pipe.gdc.height) * pipe.bds_sf;

	fov.w = (inW - (ifCropW + gdcCropW)) / inW;
	fov.h = (inH - (ifCropH + gdcCropH)) / inH;

	return fov;
}

} /* namespace */ImgUDevice::PipeConfig ImgUDevice::calculatePipeConfig(Pipe *pipe)
{
	pipeConfigs.clear();

	LOG(IPU3, Debug) << "Calculating pipe configuration for: ";
	LOG(IPU3, Debug) << "input: " << pipe->input;
	LOG(IPU3, Debug) << "main: " << pipe->main;
	LOG(IPU3, Debug) << "vf: " << pipe->viewfinder;

	const Size &in = pipe->input;

	/*
	 * \todo Filter out all resolutions < IF_CROP_MAX.
	 * See https://bugs.libcamera.org/show_bug.cgi?id=32
	 */
	if (in.width < ImgUDevice::kIFMaxCropWidth || in.height < ImgUDevice::kIFMaxCropHeight) {
		LOG(IPU3, Error) << "Input resolution " << in << " not supported";
		return {};
	}

	Size gdc = calculateGDC(pipe);

	float bdsSF = static_cast<float>(in.width) / gdc.width;
	float sf = findScaleFactor(bdsSF, bdsScalingFactors, true);

	/* Populate the configurations vector by scaling width and height. */
	unsigned int ifWidth = utils::alignUp(in.width, ImgUDevice::kIFAlignWidth);
	unsigned int ifHeight = utils::alignUp(in.height, ImgUDevice::kIFAlignHeight);
	unsigned int minIfWidth = in.width - ImgUDevice::kIFMaxCropWidth;
	unsigned int minIfHeight = in.height - ImgUDevice::kIFMaxCropHeight;
	while (ifWidth >= minIfWidth) {
		while (ifHeight >= minIfHeight) {
			Size iif{ ifWidth, ifHeight };
			calculateBDS(pipe, iif, gdc, sf);
			ifHeight -= ImgUDevice::kIFAlignHeight;
		}

		ifWidth -= ImgUDevice::kIFAlignWidth;
	}

	/* Repeat search by scaling width first. */
	ifWidth = utils::alignUp(in.width, ImgUDevice::kIFAlignWidth);
	ifHeight = utils::alignUp(in.height, ImgUDevice::kIFAlignHeight);
	minIfWidth = in.width - ImgUDevice::kIFMaxCropWidth;
	minIfHeight = in.height - ImgUDevice::kIFMaxCropHeight;
	while (ifHeight >= minIfHeight) {
		/*
		 * \todo This procedure is probably broken:
		 * https://github.com/intel/intel-ipu3-pipecfg/issues/2
		 */
		while (ifWidth >= minIfWidth) {
			Size iif{ ifWidth, ifHeight };
			calculateBDS(pipe, iif, gdc, sf);
			ifWidth -= ImgUDevice::kIFAlignWidth;
		}

		ifHeight -= ImgUDevice::kIFAlignHeight;
	}

	if (pipeConfigs.size() == 0) {
		LOG(IPU3, Error) << "Failed to calculate pipe configuration";
		return {};
	}

	FOV bestFov = calcFOV(pipe->input, pipeConfigs[0]);
	unsigned int bestIndex = 0;
	unsigned int p = 0;
	for (auto pipeConfig : pipeConfigs) {
		FOV fov = calcFOV(pipe->input, pipeConfig);
		if (fov.isLarger(bestFov)) {
			bestFov = fov;
			bestIndex = p;
		}

		++p;
	}

	LOG(IPU3, Debug) << "Computed pipe configuration: ";
	LOG(IPU3, Debug) << "IF: " << pipeConfigs[bestIndex].iif;
	LOG(IPU3, Debug) << "BDS: " << pipeConfigs[bestIndex].bds;
	LOG(IPU3, Debug) << "GDC: " << pipeConfigs[bestIndex].gdc;

	return pipeConfigs[bestIndex];
}

int main(int argc,char**argv) {
 ImgUDevice d;
 unsigned w=argc>1?std::stoul(argv[1]):832, h=argc>2?std::stoul(argv[2]):480;
 ImgUDevice::Pipe p{{1296,972},{w,h},{w,h}};
 if (argc>3) { // Exercise intermediate crop boundary and both aspect-ratio branches.
   for (unsigned height: {536u,540u,544u,972u})
    for (unsigned same: {0u,1u})
     for(float sf:bdsScalingFactors) {
       p.main=p.viewfinder=same?Size{640,480}:Size{832,480};
       pipeConfigs.clear();
       calculateBDSHeight(&p,{1296,height},p.main,1296,sf);
       for (const auto& c:pipeConfigs) {
         assert(c.iif.height>0 && c.iif.height<=height);
         assert(c.iif.height%4==0 && c.bds.height%4==0);
         assert(c.bds.height>=c.gdc.height+8);
       }
     }
   std::cout<<"boundary cases passed\n"; return 0;
 }
 auto c=d.calculatePipeConfig(&p);
 if(c.isNull()) { std::cout<<"controlled rejection\n";return 2; }
 assert(c.iif.height && c.bds.height && c.gdc.height);
 std::cout<<c.iif.width<<"x"<<c.iif.height<<" "<<c.bds.width<<"x"<<c.bds.height<<" "<<c.gdc.width<<"x"<<c.gdc.height<<" "<<c.bds_sf<<"\n";
}
