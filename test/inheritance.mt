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


class Shape {
  init(area) {
    this.area = area;
  }
}

class Circle < Shape {
  init(area) {
    super.init(area);
  }

  getRadius() {
    return (this.area / 3.1415)^(1/2);
  }
}

var c = Circle(100);
print c.getRadius();


