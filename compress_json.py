#!/usr/bin/env python

# Copyright Node.js contributors. All rights reserved.

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import json
import struct
import sys
import zlib

if __name__ == '__main__':
  fp = open(sys.argv[1])
  obj = json.load(fp)
  text = json.dumps(obj, separators=(',', ':'))
  data = zlib.compress(text, zlib.Z_BEST_COMPRESSION)

  # To make decompression a little easier, we prepend the compressed data
  # with the size of the uncompressed data as a 24 bits BE unsigned integer.
  assert len(text) < 1 << 24, 'Uncompressed JSON must be < 16 MB.'
  data = struct.pack('>I', len(text))[1:4] + data

  step = 20
  slices = (data[i:i+step] for i in xrange(0, len(data), step))
  slices = map(lambda s: ','.join(str(ord(c)) for c in s), slices)
  text = ',\n'.join(slices)

  fp = open(sys.argv[2], 'w')
  fp.write(text)
