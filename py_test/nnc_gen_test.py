import os
import sys

import subprocess
from typing import List

class nnc_test_case(object):
    def __init__(self, src_path: str) -> None:
        self.__cwd: str = os.getcwd()
        self.__source: str = src_path
        self.__init_rest()

    def __init_rest(self) -> None:
        self.pure: str = self.__source.split('.')[0]
        self.__argv: str = f'{self.pure}.argv'
        self.__stdout: str = f'{self.pure}.stdout'
        self.__retcode: str = f'{self.pure}.retcode'
        self.__chk_permissions()

    def __read_file(self, file: str) -> str:
        import io
        if not os.path.exists(file):
            return ''
        fd: io.TextIOWrapper = open(file, 'r')
        c: str = fd.read()
        fd.close()
        return c

    def __get_argv(self) -> List[str]:
        contents: str = self.__read_file(self.__argv)
        assert contents != None
        if contents.strip() == '':
            return []
        return contents.split(' ')

    def __get_retcode(self) -> int:
        contents: str = self.__read_file(self.__retcode)
        assert contents != None
        try:
            return int(contents)
        except:
            return 0

    def __get_stdout(self) -> str:
        contents: str = self.__read_file(self.__stdout)
        assert contents != None
        return contents

    def __lt__(self, other) -> bool:
        return self.pure < other.pure

    def __str__(self) -> str:
        return f'{self.pure}'

    def __chk_permissions(self) -> None:
        chk_list: List[str] = [
            f'{self.__cwd}/compile.sh', 
            f'{self.__cwd}/cleanup.sh',
            f'{self.__cwd}/prepare.sh',
        ]
        for f in chk_list:
            if not os.access(f, os.X_OK):
                os.chmod(f, 0o777)

    def __cleanup(self) -> None:
        subprocess.run([f'./cleanup.sh'])

    def __compile(self) -> str:
        p: subprocess.Popen = subprocess.Popen(
            args=[f'./compile.sh', f'{self.__source}'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        err: str = p.communicate()[1]
        ret: int = p.wait()
        if ret != 0:
            return '\ncannot compile, see error:' + f'{err}\n'
        return ''

    def __run_compiled(self) -> str:
        p: subprocess.Popen = subprocess.Popen(
            args=[f'./compiled'] + self.__get_argv(),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        report: str = ''
        out, err = p.communicate()
        ret: int = p.wait()
        e_out: str = self.__get_stdout()
        e_ret: int = self.__get_retcode()
        if e_ret != ret:
            report += f'\n\tbad retcode: expected `{e_ret}`, got `{ret}`\n'
        if e_out != out:
            report += f'\n\tbad stdout: expected `{e_out}`, got `{out}`\n'
        return report

    def run(self) -> str:
        self.__cleanup()
        report: str = ''
        report: str = self.__compile()
        if report != '':
            return report
        report = self.__run_compiled()
        if report != '':
            return report
        return 'ok'

class nnc_tester_stat(object):
    def __init__(self, whole: int) -> None:
        self.whole: int  = whole
        self.failed: int = 0
        self.succeeded: int = 0

class nnc_tester(object):
    def __init__(self, test_path: str) -> None:
        if not os.path.exists(test_path):
            raise Exception(f'{test_path} does not exists.')
        self.__path: str = test_path

    def __prepare(self) -> None:
        subprocess.run([f'./prepare.sh'])

    def discover(self) -> List[nnc_test_case]:
        tests: List[nnc_test_case] = []
        print(f'discovering tests at [{os.path.abspath(self.__path)}]', end='')
        for f in os.listdir(self.__path):
            if f.endswith('.source'):
                tests.append(nnc_test_case(os.path.join(self.__path, f)))
        tests.sort()
        print(f', found={len(tests)}')
        return tests

    def execute(self, tests: List[nnc_test_case] = None) -> nnc_tester_stat:
        if tests == None:
            tests = self.discover()
        self.__prepare()
        stat: nnc_tester_stat = nnc_tester_stat(len(tests))
        for i, test in zip(range(len(tests)), tests):
            print(f'{i+1}) {test}: {test.run()}')
        return stat
    
if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f'usage: python {os.path.basename(__file__)} [path_to_test_folder]')
    else:
        nnc_tester(sys.argv[1]).execute()