import os, sys
from typing import *

def main() -> None:
    path: str = sys.argv[1]
    name: str = sys.argv[2]
    files: List[str] = [
        f'{os.path.join(path, name)}.argv',
        f'{os.path.join(path, name)}.retcode',
        f'{os.path.join(path, name)}.stdout',
        f'{os.path.join(path, name)}.source'
    ]
    if not os.path.exists(path):
        os.mkdir(path)
    for file in files: 
        if os.path.exists(file):
            print(f"{file}: exists.")
            return
    for file in files:
        open(file, 'w').close()
        print(f"{file}: created.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'usage: python {os.path.basename(__file__)} [path] [name]')
    else:
        main()