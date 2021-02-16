class Vehicle {
  init() {
    this.engine = true;
  }

  move() {
    print "I am moving";
  }
}

class Car < Vehicle {
  init() {
    super.init();
  }
}

var car = Car();
car.move();


