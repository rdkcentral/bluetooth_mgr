import unittest

class TestJSONParsing(unittest.TestCase):
    def test_invalid_json(self):
        # Simulate JSON parsing failure
        with self.assertRaises(ValueError):
            parse_json('invalid_json')

if __name__ == '__main__':
    unittest.main()