// |reftest| skip-if(!xulRuntime.shell) -- needs drainJobQueue
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var Promise = ShellPromise;

class X {
  constructor() {
    this.value = 42;
  }
  async getValue() {
    return this.value;
  }
  setValue(value) {
    this.value = value;
  }
  async increment() {
    var value = await this.getValue();
    this.setValue(value + 1);
    return this.getValue();
  }
  async getBaseClassName() {
    return 'X';
  }
  static async getStaticValue() {
    return 44;
  }
  /* XXX: These parse, but aren't accessible yet. */
  async 10() {
    return 46;
  }
  async ["foo"]() {
    return 47;
  }
}

class Y extends X {
  async getBaseClassName() {
    return super.getBaseClassName();
  }
}

var objLiteral = {
  async get() {
    return 45;
  },
  someStuff: 5
};

var x = new X();
var y = new Y();

assertEventuallyEq(x.getValue(), 42);
assertEventuallyEq(x.increment(), 43);
/* assertEventuallyEq(x[10](), 46);
assertEventuallyEq(x.foo(), 47); */
assertEventuallyEq(X.getStaticValue(), 44);
assertEventuallyEq(objLiteral.get(), 45);
assertEventuallyEq(y.getBaseClassName(), 'X');

Promise.all([
  assertEventuallyEq(x.getValue(), 43),
  assertEventuallyEq(x.increment(), 44),
  /* assertEventuallyEq(x[10](), 46);
  assertEventuallyEq(x.foo(), 47); */
  assertEventuallyEq(X.getStaticValue(), 44),
  assertEventuallyEq(objLiteral.get(), 45),
  assertEventuallyEq(y.getBaseClassName(), 'X'),
]).then(() => {
  if (typeof reportCompare === "function")
      reportCompare(true, true);
}, () => {
  if (typeof reportCompare === "function")
      reportCompare(true, false);
});

