import subprocess
try:
    res = subprocess.run(['build/Release/mellis.exe', 'tests/test_generic_impl.ms', '--verbose'], capture_output=True, text=True)
    print("Return code:", res.returncode)
    print("STDOUT:", res.stdout)
    print("STDERR:", res.stderr)
except Exception as e:
    print(e)
