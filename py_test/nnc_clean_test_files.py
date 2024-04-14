import os
import sys

def main() -> None:
    root: str = sys.argv[1]
    abspath: str = os.path.abspath(root)
    for file in os.listdir(abspath):
        aname: str = os.path.join(abspath, file)
        bname: str = os.path.basename(aname)
        if os.path.getsize(aname) == 0:
            os.remove(aname)
            print(f'{os.path.join(root, bname)}: removed')

if __name__ == '__main__':
    if len(sys.argv) == 2:
        main()
    else:
        print(f'usage: python {os.path.basename(__file__)} [path_to_folder]')