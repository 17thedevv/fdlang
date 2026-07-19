import subprocess
try:
    subprocess.run(['cdb', '-c', 'g; k; q', 'build/Release/mellis.exe', 'tests/test_generic_impl.ms', '--verbose'], check=True, capture_output=True, text=True)
except subprocess.CalledProcessError as e:
    print("STDOUT:")
    print(e.stdout)
    print("STDERR:")
    print(e.stderr)
except Exception as e:
    print(e)
