import os

path = os.path.dirname(__file__)

flags = [
    '-std=c++11',
    '-Wall',
    '-x', 'c++',
    '-I%s/app' % path,
    '-I%s/multilink' % path,
    '-I%s/libreactor' % path,
    '-I%s/deps/json11/' % path,
]


def FlagsForFile(filename, **kwargs):
    return {
        'flags': flags,
        'do_cache': False
    }
