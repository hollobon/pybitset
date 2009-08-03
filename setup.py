from distutils.core import setup, Extension

bitset_extmodule = Extension('bitset',
                             sources=['bitsetmodule.c'])

setup(name='bitset',
      version='0.1',
      description='Bitset module.',
      ext_modules=[bitset_extmodule],
      author="Pete Hollobon",
      author_email="python@hollobon.com",
      url="http://github.com/hollobon/pybitset",
      licence="MIT"
      )

