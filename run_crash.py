import subprocess
try:
    subprocess.run(['build/Release/mellis.exe', 'tests/test_generic_impl.ms', '--verbose'], check=True, capture_output=True, text=True)
except subprocess.CalledProcessError as e:
    print(e.stdout)
    print(e.stderr)
