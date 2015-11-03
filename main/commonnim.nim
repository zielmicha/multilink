# urandom

const hexLetters = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f']

proc encodeHex(s: string): string =
  result = ""
  result.setLen(s.len * 2)
  for i in 0..s.len-1:
    var a = ord(s[i]) shr 4
    var b = ord(s[i]) and ord(0x0f)
    result[i * 2] = hexLetters[a]
    result[i * 2 + 1] = hexLetters[b]

proc urandom*(len: int): string =
  var f = open("/dev/urandom")
  defer: f.close
  result = ""
  result.setLen(len)
  let actualRead = f.readBuffer(result.cstring, len)
  if actualRead != len:
    raise newException(IOError, "cannot read random bytes")

proc hexUrandom*(len: int): string =
  urandom(len).encodeHex
