class Foo {
    
    init() {
        print "Hello from Foo!";
    }
    
}

class Bar < Foo {
    
    init() {
        super.init();
        print "Hello from Bar!";
    }
    
}

class Quux < Bar {
    
    init() {
        super.init();
        print "Hello from Quux!";
    }
    
}

Quux();
