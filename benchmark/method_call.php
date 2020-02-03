<?php
class Toggle {
var $state, $ac;

public function __construct__ ($startState, $unused) {
    $this->state = $startState;
$this->ac = 0;
  }

public function value () { return $this->state; }

public function activate () {
$this->state = !$this->state;
    return $this;
  }

}

class NthToggle extends Toggle {
var $count, $countMax;

public function __construct__ ($startState, $maxCounter) {
    parent::__construct__($startState);
    $this->countMax = $maxCounter;
    $this->count = 0;
  }

public function activate () {
    $this->count++;
    if ($this->count >= $this->countMax) {
parent::activate();
      $this->count = 0;
    }
    return $this;
  }
}


$t = microtime(true);
$n = 250000;
$val = true;
$toggle = new Toggle($val);

for ($i=0; $i<$n; $i++) {
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
  $val = $toggle->activate()->value();
}

echo $toggle->value()?'true':'false', "\r\n";

$val = true;
$ntoggle = new NthToggle($val, 3);

for ($i=0; $i<$n; $i++) {
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
  $val = $ntoggle->activate()->value();
}


$t = microtime(true) -$t;
echo $ntoggle->value()?'true':'false', "\r\n";
echo "Elapsed: $t\r\n";
?>