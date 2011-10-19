#!/usr/bin/python


from optparse import OptionParser
import os
import subprocess
import sys


"""NEXE building script

This module will take a set of source files, include paths, library paths, and
additional arguments, and use them to build.
"""


def ErrOut(text):
  """ErrOut prints an error message and the command-line that caused it.

  Prints to standard err, both the command-line normally, and separated by
  >>...<< to make it easier to copy and paste the command, or to
  find command formating issues.
  """
  sys.stderr.write('\n\n')
  sys.stderr.write( '>>>' + '>> <<'.join(sys.argv) + '<<\n\n')
  sys.stderr.write(' '.join(sys.argv) + '<<\n\n')
  sys.stderr.write(text + '\n')
  sys.exit(-1)


def MakeDir(outdir):
  if outdir and not os.path.exists(outdir):
    # There may be a race creating this directory, so ignore failure
    try:
      os.makedirs(outdir)
    except OSError:
      pass


def RemoveQuotes(opt):
  if opt and opt[0] == '"':
    return opt[1:-1]
  return opt


def ArgToList(opt):
  outlist = []
  optlist = RemoveQuotes(opt).split(' ')
  for optitem in optlist:
    optitem = RemoveQuotes(optitem)
    if optitem:
      outlist.append(optitem)
  return outlist


class Builder(object):
  """Builder object maintains options and generates build command-lines.

  The Builder object takes a set of script command-line options, and generates
  a set of paths, and command-line options for the NaCl toolchain.
  """
  def __init__(self, options):
    arch = options.arch
    build_type = options.build.split('_')
    toolname = build_type[0]
    self.outtype = build_type[1]

    if sys.platform.startswith('linux'):
      self.osname = 'linux'
    elif sys.platform.startswith('win'):
      self.osname = 'win'
    elif sys.platform.startswith('darwin'):
      self.osname = 'mac'
    else:
      ErrOut('Toolchain OS %s not supported.' % sys.platform)

    if arch  in ['x86-32', 'x86-64']:
      self.arch = arch
      self.mainarch = 'x86'
      self.subarch = arch.split('-')[1]
      tool_subdir = 'x86_64-nacl'
    else:
      ErrOut('Toolchain architecture %s not supported.' % arch)

    if toolname == 'newlib':
      toolchain = '%s_%s_newlib' % (self.osname, self.mainarch)
      self.toolname = 'newlib'
    elif toolname == 'glibc':
      toolchain = '%s_%s' % (self.osname, self.mainarch)
      self.toolname = 'glibc'
    else:
      ErrOut('Toolchain of type %s not supported.' % toolname)


    self.root_path = options.root
    self.nacl_path = os.path.join(self.root_path, 'native_client')

    project_path, project_name = os.path.split(options.name)
    self.outdir = options.objdir

    # Set the toolchain directories
    self.toolchain = self.GenNaClPath(os.path.join('toolchain', toolchain))
    self.toolbin = os.path.join(self.toolchain, tool_subdir, 'bin')
    self.toollib = os.path.join(self.toolchain, tool_subdir, 'lib'+self.subarch)
    self.toolinc = os.path.join(self.toolchain, tool_subdir, 'include')

    self.inc_paths = ArgToList(options.incdirs)
    self.lib_paths = ArgToList(options.libdirs)

    self.name = options.name
    self.BuildCompileOptions(options.compile_flags, options.defines)
    self.BuildLinkOptions(options.link_flags)
    self.BuildArchiveOptions()
    self.verbose = options.verbose
    self.suffix = options.suffix
    self.strip = options.strip

    if self.verbose:
      print 'Compile options: %s' % self.compile_options
      print 'Linker options: %s' % self.link_options

  def GenNaClPath(self, path):
    """Helper which prepends path with the native client source directory."""
    return os.path.join(self.root_path, 'native_client', path)

  def GetBinName(self, name):
    """Helper which prepends executable with the toolchain bin directory."""
    return os.path.join(self.toolbin, name)

  def BuildAssembleOptions(self, options):
    options = ArgToList(options)
    self.assemble_options = options + ['-I' + name for name in self.inc_paths]

  def BuildCompileOptions(self, options, defines):
    """Generates compile options, called once by __init__."""
    options = ArgToList(options)
    options += ['-D' + define for define in defines]
    self.compile_options = options + ['-I' + name for name in self.inc_paths]

  def BuildLinkOptions(self, options):
    """Generates link options, called once by __init__."""
    options = ArgToList(options)
    if self.toolname in ['glibc', 'newlib'] and self.mainarch == 'x86':
      options += ['-B' + self.toollib]
    self.link_options = options + ['-L' + name for name in self.lib_paths]

  def BuildArchiveOptions(self):
    """Generates link options, called once by __init__."""
    self.archive_options = []

  def Run(self, cmd_line, out):
    """Helper which runs a command line."""

    # For POSIX style path on windows for POSIX based toolchain
    cmd_line = [cmd.replace('\\', '/') for cmd in cmd_line]

    if self.verbose:
      print ' '.join(cmd_line)
    try:
      ecode = subprocess.call(cmd_line)
    except Exception, err:
      ErrOut('\n%s\nFAILED: %s\n\n' % (' '.join(cmd_line), str(err)))
    if ecode == 0:
      print '    nacl-%s %s' % (os.path.basename(cmd_line[0]), out)
    else:
      print 'Err %d: nacl-%s %s' % (ecode, os.path.basename(cmd_line[0]), out)
      print '>>%s<<' % '<< >>'.join(cmd_line)
    return ecode

  def GetObjectName(self, src):
    if self.strip:
      src = src.replace(self.strip,'')
    filepath, filename = os.path.split(src)
    filename, ext = os.path.splitext(filename)
    if self.suffix:
      return os.path.join(self.outdir, filename + '.o')
    else:
      filename = os.path.split(src)[1]
      return os.path.join(self.outdir, os.path.splitext(filename)[0] + '.o')

  def Compile(self, src):
    """Compile the source with pre-determined options."""

    filename, ext = os.path.splitext(src)
    if ext == '.c' or ext == '.S':
      bin_name = self.GetBinName('gcc')
      extra = ['-std=gnu99']
    elif ext == '.cc':
      bin_name = self.GetBinName('g++')
      extra = []
    else:
      if self.verbose:
        print 'Skipping unknown type %s.' % ext
      return None

    if self.verbose:
      print '\nCompile %s' % src

    out = self.GetObjectName(src)
    MakeDir(os.path.dirname(out))
    cmd_line = [bin_name, '-c', src, '-o', out] + extra + self.compile_options
    err = self.Run(cmd_line, out)
    if sys.platform.startswith('win') and err == 5:
      # Try again on mystery windows failure.
      err = self.Run(cmd_line, out)
    if err:
      ErrOut('\nFAILED with %d: %s\n\n' % (err, ' '.join(cmd_line)))
    return out

  def Link(self, srcs):
    """Link these objects with predetermined options and output name."""
    out = self.name
    if self.verbose:
      print '\nLink %s' % out
    bin_name = self.GetBinName('g++')
    MakeDir(os.path.dirname(out))
    cmd_line = [bin_name, '-o', out, '-Wl,--as-needed'] + srcs
    cmd_line += self.link_options
    err = self.Run(cmd_line, out)
    # TODO( Retry on windows
    if sys.platform.startswith('win') and err == 5:
      # Try again on mystery windows failure.
      err = self.Run(cmd_line, out)
    if err:
      ErrOut('\nFAILED with %d: %s\n\n' % (err, ' '.join(cmd_line)))
    return out

  def Archive(self, srcs):
    """Archive these objects with predetermined options and output name."""
    out = self.name
    if self.verbose:
      print '\nArchive %s' % out

    if '-r' in self.link_options:
      bin_name = self.GetBinName('g++')
      cmd_line = [bin_name, '-o', out, '-Wl,--as-needed'] + srcs
      cmd_line += self.link_options
    else:
      bin_name = self.GetBinName('ar')
      cmd_line = [bin_name, '-rc', out] + srcs

    MakeDir(os.path.dirname(out))
    err = self.Run(cmd_line, out)
    if sys.platform.startswith('win') and err == 5:
      # Try again on mystery windows failure.
      err = self.Run(cmd_line, out)
    if err:
      ErrOut('\nFAILED with %d: %s\n\n' % (err, ' '.join(cmd_line)))
    return out

  def Generate(self, srcs):
    """Generate final output file.

    Link or Archive the final output file, from the compiled sources.
    """
    if self.outtype == 'nexe':
      self.Link(srcs)
    elif self.outtype == 'nlib':
      self.Archive(srcs)


def Main(argv):
  parser = OptionParser()
  parser.add_option('--no-suffix', dest='suffix', default=True,
                    help='Do not append arch suffix.', action='store_false')
  parser.add_option('--sufix', dest='suffix',
                    help='Do append arch suffix.', action='store_true')
  parser.add_option('--strip', dest='strip', default='',
                    help='Strip the filename')
  parser.add_option('-a', '--arch', dest='arch',
                    help='Set target architecture')
  parser.add_option('-c', '--compile', dest='compile_only', default=False,
                    help='Compile only.', action='store_true')
  parser.add_option('-i', '--include-dirs', dest='incdirs',
                    help='Set include directories.')
  parser.add_option('-l', '--lib-dirs', dest='libdirs',
                    help='Set library directories.')
  parser.add_option('-n', '--name', dest='name',
                    help='Base path and name of the nexe.')
  parser.add_option('-o', '--objdir', dest='objdir',
                    help='Base path of the object output dir.')
  parser.add_option('-r', '--root', dest='root',
                    help='Set the root directory of the sources')
  parser.add_option('-b', '--build', dest='build',
                    help='Set build type (newlib, glibc).')
  parser.add_option('--compile_flags', dest='compile_flags',
                    help='Set compile flags.')
  parser.add_option('--link_flags', dest='link_flags',
                    help='Set link flags.')
  parser.add_option('-v', '--verbose', dest='verbose', default=False,
                    help='Enable verbosity', action='store_true')
  parser.add_option('-D', dest='defines', default=[], action='append')
  (options, files) = parser.parse_args(argv[1:])

  if not argv:
    parser.print_help()
    return -1

  build = Builder(options)
  objs = []
  for filename in files:
    out = build.Compile(filename)
    if out:
      objs.append(out)
  # Do not link if building an object
  if not options.compile_only:
    build.Generate(objs)
  return 0

if __name__ == '__main__':
  sys.exit(Main(sys.argv))
