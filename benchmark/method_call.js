class Toggle {
  constructor (startState) {
    this.state = startState;
this.ac = 0;
  }

  value () { return this.state; }
  activate () {
this.state = !this.state;
    return this;
  }

}

class NthToggle extends Toggle {
  constructor (startState, maxCounter) {
    super(startState);
    this.countMax = maxCounter;
    this.count = 0;
  }

  activate () {
    this.count++;
    if (this.count >= this.countMax) {
      super.activate();
      this.count = 0;
    }
    return this;
  }
}

var t = new Date().getTime();
var n = 250000
var val = true
var toggle = new Toggle(val)

for (var i=0; i<n; i++) {
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
  val = toggle.activate().value();
}

console.log(toggle.value());

val = true
var ntoggle = new NthToggle(val, 3);

for (var i=0; i<n; i++) {
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
  val = ntoggle.activate().value();
}

t = (new Date().getTime() -t)/1000.0;
console.log(ntoggle.value());
console.log("Elapsed: " +t);