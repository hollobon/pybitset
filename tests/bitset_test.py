import unittest
import pickle

from bitset import Bitset

class TestBitset(unittest.TestCase):
    def setUp(self):
        self.l1 = [1,2,3,4,8,9,32]
        self.b1 = Bitset(self.l1)
        self.b2 = Bitset([2,3,4,6,9])
        self.b3 = Bitset([2,3,4,32])
        self.b4 = Bitset([5,7,15])
        self.b5 = Bitset(xrange(1, 33))
        self.b6 = Bitset()

    def testinit(self):
        self.assertEqual(Bitset([1,2,3,4]), Bitset([1,2,3,4]))
        self.assertEqual(Bitset(set([1,2,3,4])), Bitset([1,2,3,4]))
        self.assertEqual(Bitset({1: 10, 2: 20, 3: 30, 4: 40}), Bitset([1,2,3,4]))
        self.assertRaises(TypeError, lambda: Bitset(1))
        self.assertRaises(TypeError, lambda: Bitset([33]))
        self.assertRaises(TypeError, lambda: Bitset([0]))
        self.assertRaises(TypeError, lambda: Bitset(["a"]))
        self.assertRaises(TypeError, lambda: Bitset("test"))

    def testin(self):
        self.assertTrue(2 in self.b2)
        self.assertTrue(1 not in self.b2)
        self.assertFalse(1 in self.b2)
        self.assertFalse(2 not in self.b2)

        for x in self.l1:
            self.assertTrue(x in self.b1)

    def testiter(self):
        self.assertEqual(list(self.b1), self.l1)

    def testclear(self):
        self.assertNotEqual(self.b1, Bitset())
        self.b1.clear()
        self.assertEqual(self.b1, Bitset())
        
        self.assertEqual(self.b6, Bitset())
        self.b6.clear()
        self.assertEqual(self.b6, Bitset())

    def testremove(self):
        self.b1.remove(32)
        self.assertEqual(self.b1, Bitset([1, 2, 3, 4, 8, 9]))

        self.assertRaises(KeyError, lambda: self.b1.remove(31))

    def testadd(self):
        self.b1.add(31)
        self.assertEqual(self.b1, Bitset([1, 2, 3, 4, 8, 9, 31, 32]))
        self.b6.add(6)
        self.assertEqual(self.b6, Bitset([6]))
        self.b4.add(5)
        self.assertEqual(self.b4, Bitset([5, 7, 15]))
        self.assertEqual(self.b4.add(1), None)
        self.assertRaises(TypeError, lambda: self.b4.add("test"))

    def testdiscard(self):
        self.b1.discard(31)
        self.assertEqual(self.b1, Bitset(self.l1))

        self.b1.discard(32)
        self.assertEqual(self.b1, Bitset([1, 2, 3, 4, 8, 9]))

    def testpop(self):
        for i in range(len(self.l1)):
            self.assertTrue(self.b1.pop() in self.l1)

        for i in xrange(1, 33):
            v = self.b5.pop()
            self.assertTrue(0 < v)
            self.assertTrue(v < 33)

    def testintersection(self):
        self.assertEqual(self.b2.intersection(self.b1), self.b1.intersection(self.b2))
        self.assertEqual(self.b1.intersection(self.b2), Bitset([2,3,4,9]))
        self.assertEqual(self.b1.intersection(self.b4), Bitset())
                         
    def testunion(self):
        self.assertEqual(self.b1.union(self.b1), self.b1)
        self.assertNotEqual(self.b1.union(self.b1), self.b2)
        self.assertEqual(self.b2.union(self.b3), Bitset([2, 3, 4, 6, 9, 32]))

    def testdifference(self):
        self.assertEqual(self.b1.difference(self.b2), Bitset([1, 8, 32]))

    def testsymmetric_difference(self):
        self.assertEqual(self.b6.symmetric_difference(self.b6), Bitset())
        self.assertEqual(self.b1.symmetric_difference(self.b1), Bitset())
        self.assertEqual(self.b1.symmetric_difference(self.b2), Bitset([1, 6, 8, 32]))

    def testisdisjoint(self):
        self.assertTrue(self.b1.isdisjoint(self.b4))
        self.assertFalse(self.b1.isdisjoint(self.b2))

    def testissubset(self):
        self.assertTrue(self.b3.issubset(self.b1))
        self.assertFalse(self.b2.issubset(self.b1))

    def testissuperset(self):
        self.assertTrue(self.b1.issuperset(self.b3))
        self.assertFalse(self.b1.issuperset(self.b2))

    def testpickle(self):
        for o in (self.b1, self.b2, self.b3, self.b4, self.b5, self.b6):
            for protocol in (None, 1, 2):
                self.assertEqual(pickle.loads(pickle.dumps(o, protocol=protocol)), o)

    def testlen(self):
        self.assertEqual(len(self.b1), len(self.l1))
        self.assertEqual(len(self.b2), len(list(self.b2)))

    def testintersection_update(self):
        c = Bitset(self.b2)
        c.intersection_update(self.b1)        
        self.assertEqual(c, self.b2.intersection(self.b1))

        c = Bitset(self.b1)
        c.intersection_update(self.b2)
        self.assertEqual(c, self.b1.intersection(self.b2))

        c = Bitset(self.b1)
        c.intersection_update(self.b4)
        self.assertEqual(c, self.b1.intersection(self.b4))
                         
    def testupdate(self):
        c = Bitset(self.b1)
        c.update(self.b1)
        self.assertEqual(c, self.b1.union(self.b1))

        c = Bitset(self.b2)
        c.update(self.b3)
        self.assertEqual(c, self.b2.union(self.b3))

    def testdifference_update(self):
        c = Bitset(self.b1)
        c.difference_update(self.b2)
        self.assertEqual(c, self.b1.difference(self.b2))

    def testsymmetric_difference_update(self):
        c = Bitset(self.b1)
        c.symmetric_difference_update(self.b2)
        self.assertEqual(c, self.b1.symmetric_difference(self.b2))

    def testeq(self):
        self.assertTrue(self.b1 == Bitset(self.b1))
        self.assertFalse(self.b1 == set(self.b1))
        self.assertFalse(self.b1 == self.b2)
        
    def testne(self):
        self.assertFalse(self.b1 != Bitset(self.b1))
        self.assertTrue(self.b1 != set(self.b1))
        self.assertTrue(self.b1 != self.b2)
        
    def testlt(self):
        self.assertTrue(self.b3 < self.b1)
        self.assertFalse(self.b2 < self.b1)
        self.assertFalse(self.b2 < self.b2)
        self.assertRaises(TypeError, lambda: self.b3 < set(self.b1))

    def testlte(self):
        self.assertTrue(self.b3 <= self.b1)
        self.assertFalse(self.b2 <= self.b1)
        self.assertTrue(self.b2 <= self.b2)
        self.assertRaises(TypeError, lambda: self.b3 <= set(self.b1))

    def testgt(self):
        self.assertTrue(self.b1 > self.b3)
        self.assertFalse(self.b1 > self.b2)
        self.assertFalse(self.b2 > self.b2)
        self.assertRaises(TypeError, lambda: self.b1 > set(self.b3))

    def testgte(self):
        self.assertTrue(self.b1 >= self.b3)
        self.assertFalse(self.b1 >= self.b2)
        self.assertTrue(self.b2 >= self.b2)
        self.assertRaises(TypeError, lambda: self.b1 >= set(self.b3))
        
    def testand(self):
        self.assertEqual(self.b1 & self.b2, self.b1.intersection(self.b2))
        self.assertEqual(self.b1 & self.b3, self.b1.intersection(self.b3))
        self.assertEqual(self.b4 & self.b3, self.b4.intersection(self.b3))
        self.assertEqual(self.b5 & self.b6, self.b5.intersection(self.b6))
        self.assertRaises(TypeError, lambda: self.b1 & set([1]))

    def testor(self):
        self.assertEqual(self.b1 | self.b2, self.b1.union(self.b2))
        self.assertEqual(self.b1 | self.b3, self.b1.union(self.b3))
        self.assertEqual(self.b4 | self.b3, self.b4.union(self.b3))
        self.assertEqual(self.b5 | self.b6, self.b5.union(self.b6))
        self.assertRaises(TypeError, lambda: self.b1 | set([1]))

    def testxor(self):
        self.assertEqual(self.b1 ^ self.b2, self.b1.symmetric_difference(self.b2))
        self.assertEqual(self.b1 ^ self.b3, self.b1.symmetric_difference(self.b3))
        self.assertEqual(self.b4 ^ self.b3, self.b4.symmetric_difference(self.b3))
        self.assertEqual(self.b5 ^ self.b6, self.b5.symmetric_difference(self.b6))
        self.assertRaises(TypeError, lambda: self.b1 ^ set([1]))

    def testsub(self):
        self.assertEqual(self.b1 - self.b2, self.b1.difference(self.b2))
        self.assertEqual(self.b1 - self.b3, self.b1.difference(self.b3))
        self.assertEqual(self.b4 - self.b3, self.b4.difference(self.b3))
        self.assertEqual(self.b5 - self.b6, self.b5.difference(self.b6))
        self.assertRaises(TypeError, lambda: self.b1 - set([1]))

    def testiand(self):
        c = Bitset(self.b1)
        c &= Bitset(self.b2)
        self.assertEqual(c, self.b1 & self.b2)

        def _testiand():
            c = Bitset(self.b1)
            c &= set([1])

        self.assertRaises(TypeError, _testiand)

    def testior(self):
        c = Bitset(self.b1)
        c |= Bitset(self.b2)
        self.assertEqual(c, self.b1 | self.b2)

        def _testior():
            c = Bitset(self.b1)
            c |= set([1])

        self.assertRaises(TypeError, _testior)

    def testixor(self):
        c = Bitset(self.b1)
        c ^= Bitset(self.b2)
        self.assertEqual(c, self.b1 ^ self.b2)

        def _testixor():
            c = Bitset(self.b1)
            c ^= set([1])

        self.assertRaises(TypeError, _testixor)

    def testisub(self):
        c = Bitset(self.b1)
        c -= Bitset(self.b2)
        self.assertEqual(c, self.b1 - self.b2)

        def _testisub():
            c = Bitset(self.b1)
            c -= set([1])

        self.assertRaises(TypeError, _testisub)

if __name__ == '__main__':
    import sys
    unittest.main()
