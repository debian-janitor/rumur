#!/usr/bin/env python3

'''
integration test suite

Despite using Python’s unittest, this is not a set of unit tests. The Python
module is simply a nice low-overhead testing framework.
'''

import codecs
import multiprocessing
import os
from pathlib import Path
import re
import subprocess as sp
import sys
import tempfile
from typing import Any, Dict, Generator, List, Optional, Tuple
import unittest

CPUS = multiprocessing.cpu_count()

VERIFIER_RNG = Path(__file__).resolve().parent / '../misc/verifier.rng'
MURPHI2XML_RNG = Path(__file__).resolve().parent / '../misc/murphi2xml.rng'

# test configuration variables, set during main
CONFIG: Dict[str, Any] = {}

def enc(s): return s.encode('utf-8', 'replace')
def dec(s): return s.decode('utf-8', 'replace')

def run(args: List[str], stdin: Optional[str] = None) -> Tuple[int, str, str]:
  '''
  run a command and return its result
  '''
  if stdin is not None:
    stdin = enc(stdin)
  env = {k: v for k, v in os.environ.items()}
  env.update({k: str(v) for k, v in CONFIG.items()})
  p = sp.run(args, stdout=sp.PIPE, stderr=sp.PIPE, input=stdin, env=env)
  return p.returncode, dec(p.stdout), dec(p.stderr)

def parse_test_options(src: Path, debug: bool = False,
                       multithreaded: bool = False, xml: bool = False) -> \
    Generator[Tuple[str, Any], None, None]:
  '''
  extract test tweaks and directives from leading comments in a test input
  '''
  with open(src, 'rt', encoding='utf-8') as f:
    for line in f:
      # recognise '-- rumur_flags: …' etc lines
      m = re.match(r'\s*--\s*(?P<key>[a-zA-Z_]\w*)\s*:(?P<value>.*)$', line)
      if m is None:
        break
      yield m.group('key'), eval(m.group('value').strip())

class executable(unittest.TestCase):
  '''
  test cases involving running a custom executable file
  '''

  def _run(self, testcase: Path):

    assert os.access(testcase, os.X_OK), \
      f'non-executable test case {testcase} attached to executable class'

    ret, stdout, stderr = run([testcase])
    output = f'{stdout}{stderr}'
    if ret == 125:
      self.skipTest(output.strip())
    elif ret != 0:
      self.fail(output)

class murphi2c(unittest.TestCase):
  '''
  test cases for murphi2c
  '''

  def _run(self, testcase: Path):

    tweaks = {k: v for k, v in parse_test_options(testcase)}

    # there is no C equivalent of isundefined, because an implicit assumption in
    # the C representation is that you do not rely on undefined values
    with open(testcase, 'rt', encoding='utf-8') as f:
      should_fail = re.search(r'\bisundefined\b', f.read()) is not None

    args = ['murphi2c', testcase]
    if CONFIG['HAS_VALGRIND']:
      args = ['valgrind', '--leak-check=full', '--show-leak-kinds=all',
        '--error-exitcode=42'] + args
    ret, stdout, stderr = run(args)
    if CONFIG['HAS_VALGRIND']:
      if ret == 42:
        self.fail(f'Memory leak:\n{stdout}{stderr}')

    # if rumur was expected to reject this model, we allow murphi2c to fail
    if tweaks.get('rumur_exit_code', 0) == 0 and not should_fail and ret != 0:
      self.fail(f'Unexpected murphi2c exit status {ret}:\n{stdout}{stderr}')

    if should_fail and ret == 0:
      self.fail(f'Unexpected murphi2c exit status {ret}:\n{stdout}{stderr}')

    if ret != 0:
      return

    # ask the C compiler if this is valid
    args = [CONFIG['CC']] + CONFIG['C_FLAGS'] + ['-c', '-o', os.devnull, '-']
    ret, out, err = run(args, stdout)
    if ret != 0:
      self.fail(f'C compilation failed:\n{out}{err}\nProgram:\n{stdout}')

class murphi2cHeader(unittest.TestCase):
  '''
  test cases for murphi2c --header
  '''

  def _run(self, testcase: Path):

    tweaks = {k: v for k, v in parse_test_options(testcase)}

    # there is no C equivalent of isundefined, because an implicit assumption in
    # the C representation is that you do not rely on undefined values
    with open(testcase, 'rt', encoding='utf-8') as f:
      should_fail = re.search(r'\bisundefined\b', f.read()) is not None

    args = ['murphi2c', '--header', testcase]
    if CONFIG['HAS_VALGRIND']:
      args = ['valgrind', '--leak-check=full', '--show-leak-kinds=all',
        '--error-exitcode=42'] + args
    ret, stdout, stderr = run(args)
    if CONFIG['HAS_VALGRIND']:
      if ret == 42:
        self.fail(f'Memory leak:\n{stdout}{stderr}')

    # if rumur was expected to reject this model, we allow murphi2c to fail
    if tweaks.get('rumur_exit_code', 0) == 0 and not should_fail and ret != 0:
      self.fail(f'Unexpected murphi2c exit status {ret}:\n{stdout}{stderr}')

    if should_fail and ret == 0:
      self.fail(f'Unexpected murphi2c exit status {ret}:\n{stdout}{stderr}')

    if ret != 0:
      return

    with tempfile.TemporaryDirectory() as tmp:

      # write the header to a temporary file
      header = Path(tmp) / 'header.h'
      with open(header, 'wt', encoding='utf-8') as f:
        f.write(stdout)

      # ask the C compiler if the header is valid
      main_c = f'#include "{header}"\nint main(void) {{ return 0; }}\n'
      args = [CONFIG['CC']] + CONFIG['C_FLAGS'] + ['-o', os.devnull, '-']
      ret, stdout, stderr = run(args, main_c)
      if ret != 0:
        self.fail(f'C compilation failed:\n{stdout}{stderr}')

      # ask the C++ compiler if it is valid there too
      ret, stdout, stderr = run([CONFIG['CXX'], '-std=c++11', '-o', os.devnull,
        '-x', 'c++', '-', '-Werror=format', '-Werror=sign-compare',
        '-Werror=type-limits'], main_c)
      if ret != 0:
        self.fail(f'C++ compilation failed:\n{stdout}{stderr}')

class murphi2uclid(unittest.TestCase):
  '''
  test cases for murphi2uclid
  '''

  def _run(self, testcase: Path):

    tweaks = {k: v for k, v in parse_test_options(testcase)}

    # there is no Uclid5 equivalent of the modulo operator (%)
    with open(testcase, 'rt', encoding='utf-8') as f:
      should_fail = '%' in f.read()

    args = ['murphi2uclid', testcase]
    if CONFIG['HAS_VALGRIND']:
      args = ['valgrind', '--leak-check=full', '--show-leak-kinds=all',
        '--error-exitcode=42'] + args
    ret, stdout, stderr = run(args)
    if CONFIG['HAS_VALGRIND']:
      if ret == 42:
        self.fail(f'Memory leak:\n{stdout}{stderr}')

    # if rumur was expected to reject this model, we allow murphi2uclid to fail
    if tweaks.get('rumur_exit_code', 0) == 0 and not should_fail and ret != 0:
      self.fail(f'Unexpected murphi2uclid exit status {ret}:\n{stdout}{stderr}')

    if should_fail and ret == 0:
      self.fail(f'Unexpected murphi2uclid exit status {ret}:\n{stdout}{stderr}')

    if ret != 0:
      return

    # if we do not have Uclid5 available, skip the remainder of the test
    if not CONFIG['HAS_UCLID']:
      self.skipTest('uclid not available for validation')

    with tempfile.TemporaryDirectory() as tmp:

      # write the Uclid5 source to a temporary file
      src = Path(tmp) / 'source.ucl'
      with open(src, 'wt', encoding='utf-8') as f:
        f.write(stdout)

      # ask Uclid if the source is valid
      ret, stdout, stderr = run(['uclid', src])
      if ret != 0:
        self.fail(f'uclid failed:\n{stdout}{stderr}')

class murphi2xml(unittest.TestCase):
  '''
  test cases for murphi2xml
  '''

  def _run(self, testcase: Path):

    tweaks = {k: v for k, v in parse_test_options(testcase)}

    args = ['murphi2xml', testcase]
    if CONFIG['HAS_VALGRIND']:
      args = ['valgrind', '--leak-check=full', '--show-leak-kinds=all',
        '--error-exitcode=42'] + args
    ret, stdout, stderr = run(args)
    if CONFIG['HAS_VALGRIND']:
      if ret == 42:
        self.fail(f'Memory leak:\n{stdout}{stderr}')

    # if rumur was expected to reject this model, we allow murphi2xml to fail
    if tweaks.get('rumur_exit_code', 0) == 0 and ret != 0:
      self.fail(f'Unexpected murphi2xml exit status {ret}:\n{stdout}{stderr}')

    if ret != 0:
      return

    # murphi2xml will have written XML to its stdout
    xmlcontent = stdout

    # See if we have xmllint
    if not CONFIG['HAS_XMLLINT']:
      self.skipTest('xmllint not available for validation')

    # Validate the XML
    ret, stdout, stderr = run(['xmllint', '--relaxng', MURPHI2XML_RNG,
      '--noout', '-'], xmlcontent)
    if ret != 0:
      self.fail(f'Failed to validate:\n{stdout}{stderr}')

class rumur(unittest.TestCase):
  '''
  test cases involving generating a checker and running it
  '''

  def _run_param(self, testcase: Path, debug: bool, optimised: bool,
                 multithreaded: bool, xml: bool):

    tweaks = {k: v for k, v in parse_test_options(testcase, debug,
      multithreaded, xml)}

    if tweaks.get('skip_reason') is not None:
      self.skipTest(tweaks['skip_reason'])

    # build up arguments to call rumur
    args = ['rumur', '--output', '/dev/stdout', testcase]
    if debug: args += ['--debug']
    if xml: args += ['--output-format', 'machine-readable']
    if multithreaded and CPUS == 1: args +=['--threads', '2']
    elif not multithreaded: args += ['--threads', '1']
    args += tweaks.get('rumur_flags', [])

    if CONFIG['HAS_VALGRIND']:
      args = ['valgrind', '--leak-check=full', '--show-leak-kinds=all',
        '--error-exitcode=42'] + args

    # call rumur
    ret, stdout, stderr = run(args)
    if CONFIG['HAS_VALGRIND']:
      if ret == 42:
        self.fail(f'Memory leak:\n{stdout}{stderr}')
    if ret != tweaks.get('rumur_exit_code', 0):
      self.fail(f'Rumur failed:\n{stdout}{stderr}')

    # if we expected to fail, we are done
    if ret != 0: return

    model_c = stdout

    with tempfile.TemporaryDirectory() as tmp:

      # build up arguments to call the C compiler
      model_bin = Path(tmp) / 'model.exe'
      args = [CONFIG['CC']] + CONFIG['C_FLAGS']
      if optimised: args += ['-O3']
      args += ['-o', model_bin, '-', '-lpthread']

      if CONFIG['NEEDS_LIBATOMIC']:
        args += ['-latomic']

      # call the C compiler
      ret, stdout, stderr = run(args, model_c)
      if ret != 0:
        self.fail(f'C compilation failed:\n{stdout}{stderr}')

      # now run the model itself
      ret, stdout, stderr = run([model_bin])
      if ret != tweaks.get('checker_exit_code', 0):
        self.fail(f'Unexpected checker exit status {ret}:\n{stdout}{stderr}')

    # if the test has a stdout expectation, check that now
    if tweaks.get('checker_output') is not None:
      if tweaks['checker_output'].search(stdout) is None:
        self.fail( 'Checker output did not match expectation regex:\n'
                  f'{stdout}{stderr}')

    # coarse grained check for whether the model contains a 'put' statement that
    # could screw up XML validation
    with open(testcase, 'rt', encoding='utf-8') as f:
      has_put = re.search(r'\bput\b', f.read()) is not None

    if xml and not has_put:

      model_xml = stdout

      if not CONFIG['HAS_XMLLINT']: self.skipTest('xmllint not available')

      # validate the XML
      args = ['xmllint', '--relaxng', VERIFIER_RNG, '--noout', '-']
      ret, stdout, stderr = run(args, model_xml)
      if ret != 0:
        self.fail( 'Failed to XML-validate machine reachable output:\n'
                  f'{stdout}{stderr}')

class rumurSingleThreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, False, False, False)

class rumurDebugSingleThreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, True, False, False, False)

class rumurOptimisedSingleThreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, True, False, False)

class rumurDebugOptimisedSingleThreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, True, True, False, False)

class rumurMultithreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, False, True, False)

class rumurDebugMultithreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, True, False, True, False)

class rumurOptimisedMultithreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, True, True, False)

class rumurDebugOptimisedMultithreaded(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, True, True, True, False)

class rumurSingleThreadedXML(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, False, False, True)

class rumurOptimisedSingleThreadedXML(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, True, False, True)

class rumurMultithreadedXML(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, False, True, True)

class rumurOptimisedMultithreadedXML(rumur):
  def _run(self, testcase: Path):
    self._run_param(testcase, False, True, True, True)

def make_name(t: Path) -> str:
  '''
  name mangle a path into a valid test case name
  '''
  safe_name = re.sub(r'[^a-zA-Z0-9]', '_', t.name)
  return f'test_{safe_name}'

def main():

  # setup stdout to make encoding errors non-fatal
  sys.stdout = codecs.getwriter('utf-8')(sys.stdout.buffer, 'replace')

  # parse configuration
  global CONFIG
  for p in sorted((Path(__file__).parent / 'config').iterdir()):

    # skip subdirectories
    if p.is_dir(): continue

    # skip non-executable files
    if not os.access(p, os.X_OK): continue

    CONFIG[p.name] = eval(sp.check_output([p]))

  # find files in our directory
  root = Path(__file__).parent
  for p in sorted(root.iterdir()):

    # skip directories
    if p.is_dir(): continue

    # skip ourselves
    if p.samefile(__file__): continue

    name = make_name(p)

    # if this is executable, treat it as a test case
    if os.access(p, os.X_OK):
      assert not hasattr(executable, name), \
        f'name collision involving executable.{name}'
      setattr(executable, name, lambda self, p=p: self._run(p))

    # if this is not a model, skip the remaining generic logic
    if p.suffix != '.m': continue

    for c in (rumurSingleThreaded,
              rumurDebugSingleThreaded,
              rumurOptimisedSingleThreaded,
              rumurDebugOptimisedSingleThreaded,
              rumurMultithreaded,
              rumurDebugMultithreaded,
              rumurOptimisedMultithreaded,
              rumurDebugOptimisedMultithreaded,
              rumurSingleThreadedXML,
              rumurOptimisedSingleThreadedXML,
              rumurMultithreadedXML,
              rumurOptimisedMultithreadedXML,
             ):
      assert not hasattr(c, name), f'name collision involving rumur.{name}'
      setattr(c, name, lambda self, p=p: self._run(p))

    assert not hasattr(murphi2c, name), \
      f'name collision involving murphi2c.{name}'
    setattr(murphi2c, name, lambda self, p=p: self._run(p))

    assert not hasattr(murphi2cHeader, name), \
      f'name collision involving murphi2cHeader.{name}'
    setattr(murphi2cHeader, name, lambda self, p=p: self._run(p))

    assert not hasattr(murphi2uclid, name), \
      f'name collision involving murphi2uclid.{name}'
    setattr(murphi2uclid, name, lambda self, p=p: self._run(p))

    assert not hasattr(murphi2xml, name), \
      f'name collision involving murphi2xml.{name}'
    setattr(murphi2xml, name, lambda self, p=p: self._run(p))

  unittest.main()

if __name__ == '__main__':
  main()
