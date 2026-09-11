import hashlib
import importlib.util
from pathlib import Path
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('doctor', Path(__file__).parents[1] / 'doctor/libcamera-doctor.py')
doctor = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(doctor)


class DoctorTest(unittest.TestCase):
    def setUp(self):
        self.versions = {name: 'tested' for name in doctor.PACKAGES}
        self.baseline = {'packages': self.versions, 'sha256': {'/usr/lib/example': hashlib.sha256(b'patched').hexdigest()}}

    def test_missing_and_mixed_packages_fail(self):
        self.assertEqual(doctor.package_result({})[0], 'FAIL')
        self.assertEqual(doctor.package_result(dict(self.versions, libcamera='new'))[0], 'FAIL')

    def test_new_consistent_version_is_not_assumed_fixed_or_broken(self):
        versions = {name: 'new' for name in doctor.PACKAGES}
        self.assertEqual(doctor.package_result(versions)[0], 'PASS')
        self.assertEqual(doctor.fingerprint_result(versions, self.baseline)[0], 'REVIEW')

    def test_same_version_needs_actual_matching_binaries(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / 'usr/lib/example'
            path.parent.mkdir(parents=True)
            self.assertEqual(doctor.fingerprint_result(self.versions, self.baseline, root)[0], 'REVIEW')
            path.write_bytes(b'patched')
            self.assertEqual(doctor.fingerprint_result(self.versions, self.baseline, root)[0], 'PASS')
            path.write_bytes(b'unpatched')
            self.assertEqual(doctor.fingerprint_result(self.versions, self.baseline, root)[0], 'REVIEW')


if __name__ == '__main__':
    unittest.main()
