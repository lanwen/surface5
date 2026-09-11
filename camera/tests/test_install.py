from pathlib import Path
import subprocess
import tempfile
import unittest

INSTALL = Path(__file__).resolve().parents[1] / 'virtual-camera/install.py'


class InstallerTest(unittest.TestCase):
    def call(self, root, *args, ok=True):
        result = subprocess.run(['python3', str(INSTALL), *args, '--root', str(root)], capture_output=True, text=True)
        self.assertEqual(result.returncode == 0, ok, result.stdout + result.stderr)

    def test_system_conflict_backup_and_exact_restore(self):
        with tempfile.TemporaryDirectory(prefix='surface5 with spaces ') as directory:
            root = Path(directory)
            old = root / 'etc/modprobe.d/surface-virtual-camera.conf'
            old.parent.mkdir(parents=True)
            old.write_text('existing settings\n')
            self.call(root, 'install-system', ok=False)
            self.assertEqual(old.read_text(), 'existing settings\n')
            self.call(root, 'install-system', '--replace')
            self.assertIn('video_nr=42', old.read_text())
            self.call(root, 'install-system')
            self.call(root, 'uninstall-system')
            self.assertEqual(old.read_text(), 'existing settings\n')

    def test_user_roundtrip_and_modified_file_protection(self):
        with tempfile.TemporaryDirectory(prefix='surface5 with spaces ') as directory:
            root = Path(directory)
            self.call(root, 'install-user')
            home = root / str(Path.home()).lstrip('/')
            service = home / '.config/systemd/user/surface-camera-demand.service'
            original = service.read_text()
            self.assertIn('%h/.local/lib/surface5-camera/sink.py', original)
            service.write_text('user edit')
            self.call(root, 'uninstall-user', ok=False)
            self.assertEqual(service.read_text(), 'user edit')
            service.write_text(original)
            self.call(root, 'uninstall-user')
            self.assertFalse(service.exists())
