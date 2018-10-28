Sequence::all = $$f{
for x in this: if !f(x): return false
return true
}

Sequence::any = $$f{
for x in this: if f(x): return true
return false;
}

Sequence::none = $$f: !this.any(f)

Sequence::find = $$f{
for x in this {
if f(x): return x
}}

Sequence::count = $$f{
var count = 0
for x in this: if x==f: count+=1
return count
}

Sequence::countIf = $$f{
var count = 0
for x in this: if f(x): count+=1
return count
}

Sequence::in = $$f{
for x in this: if x==f: return true
return false
}

Sequence::first = $$: for i in this: return i

Sequence::last = $${
let val = null
for x in this: val=x
return val
}

Sequence::toList = $$: List.of(this)
Sequence::toTuple = $$: Tuple.of(this)
Sequence::toSet = $$: Set.of(this)

Sequence::map = $$f: f(x) for x in this

Sequence::filter = $$f: x for x in this if f(x)

Sequence::reduce = $$(f,a){
for x in this: a = a==null ? x : f(a,x)
return a
}

Sequence::min = $$(f = $(a,b): a<b?a:b): this.reduce(f)
Sequence::max = $$(f = $(a,b): a>b?a:b): this.reduce(f)

Sequence::dropWhile = $$f: $*{
for x in this {
if f(x) break
yield x
}}

Sequence::skipWhile = $$f: $*{
var test = true
for x in this {
if test && f(x) continue
test = false
yield x
}}

Sequence::skip = $$n: $*{
let i = 0
for x in this {
if i>=n: yield x
i+=1
}}

Sequence::limit = $$n: $*{
let i=0
for x in this {
if i>=n: break
i+=1
yield x
}}

Sequence::enumerate = $$(n=0): $*{
for x in this {
yield (n, x)
n+=1
}}

Sequence::zipWith = $(seqs...): $*{
var iterators = [], keys = [], values = []
for seq in seqs { iterators.push(seq.iterator); keys.push(null); values.push(null); }
while true {
print('Iteration, ' + iterators.length + ' iterators')
for i in 0..iterators.length {
print('Iterator ' + i + ': old key = ' + keys[i])
keys[i] = iterators[i].iterate(keys[i])
print('Iterator ' + i + ': new key = ' + keys[i])
if keys[i]==null {
print('Key is null!')
return null
}
print('Iterator ' + i + ': old value = ' + values[i])
values[i] = iterators[i].iteratorValue(keys[i])
print('Iterator ' + i + ': new value = ' + values[i])
}
let re = values.toTuple
print('Yielding ' + re.toString)
yield re
}
print('End')
}

Map::keys = $$: k for k, v in this

Map::values = $$: v for k, v in this
