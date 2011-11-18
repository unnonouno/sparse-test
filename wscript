subdirs = 'src'

def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  conf.env.CXXFLAGS += ['-O2', '-Wall', '-g', '-pipe']

  conf.load('compiler_cxx')
  conf.check_cfg(package = 'pficommon', args = '--cflags --libs')

  if conf.check_cxx(header_name = 'boost/numeric/ublas/vector.hpp',
                    mandatory = False):
    conf.env.HAVE_UBLAS = True

  if conf.check_cfg(package = 'eigen3', args = '--cflags --libs',
                    mandatory = False):
    conf.env.HAVE_EIGEN3 = True
  elif conf.check_cfg(package = 'eigen2', args = '--cflags --libs',
                      mandatory = False):
    conf.env.HAVE_EIGEN2 = True

  if conf.check_cxx(header_name = 'dag/dag_vector.hpp',
                    mandatory = False):
    conf.env.HAVE_DAG_VECTOR = True

  conf.write_config_header('config.h')

def build(bld):
    use = ['PFICOMMON']
    if 'HAVE_EIGEN3' in bld.env:
        use.append('EIGEN3')
    elif 'HAVE_EIGEN2' in bld.env:
        use.append('EIGEN2')

    bld.program(
        source = 'sparse-test.cpp',
        target = 'sparse-test',
        includes = '.',
        use = use
        )
