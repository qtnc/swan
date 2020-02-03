// Ported from the Python version.

class Tree {
  constructor (item, depth) {
this.item = item;
    if (depth > 0) {
      var item2 = item + item;
      depth--;
      this.left = new Tree(item2 - 1, depth);
this.right = new Tree(item2, depth);
    }
  }

  check () {
if (!this.left) {
      return this.item;
    }
    return this.item + this.left.check() - this.right.check();
  }
}

var minDepth = 4;
var maxDepth = 12;
var stretchDepth = maxDepth + 1;

var start = new Date().getTime();

console.log("stretch tree of depth " + stretchDepth + " check: " +
    "" + new Tree(0, stretchDepth).check())

var longLivedTree = new Tree(0, maxDepth);

var  iterations = (2 ** maxDepth) ;

var depth = minDepth;
while (depth < stretchDepth) {
  var check = 0;
  for (var i=0; i<iterations; i++)  {
    check += new Tree(i, depth).check() + new Tree(-i, depth).check();
  }

console.log("" + (iterations * 2) + " trees of depth " + depth + " check: " + check);
  iterations /= 4;
  depth += 2;
}

console.log(    "long lived tree of depth " + maxDepth + " check: " + longLivedTree.check())

start = (new Date().getTime()-start)/1000.0;
console.log("Elapsed: " + start);
