import unittest
import os
import sys

BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

from python_wrapper import NeuroDBConnector, AITranslator, QueryValidator, QueryLogger

class TestNeuroDB(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.connector = NeuroDBConnector()
        cls.translator = AITranslator()
        cls.logger = QueryLogger()

    def test_schema_retrieval(self):
        schema = self.connector.get_schema()
        self.assertIsInstance(schema, list)
        table_names = [t['name'] for t in schema]
        self.assertIn("students", table_names)

    def test_sql_query_execution(self):
        res = self.connector.execute_query("SELECT * FROM students;")
        self.assertTrue(res["success"])
        self.assertGreater(res["row_count"], 0)
        self.assertIn("name", res["dataframe"].columns)

    def test_ai_translator_rule_fallback(self):
        res = self.translator.translate("show students with marks greater than 80")
        self.assertIn("SELECT * FROM students WHERE marks > 80", res["sql"])
        self.assertEqual(res["mode"], "Rule-Based Engine")

    def test_query_validator_typo(self):
        val = QueryValidator.validate("SELEKT * FROM students;")
        self.assertTrue(val["is_valid"])
        self.assertTrue(val["has_corrections"])
        self.assertEqual(val["corrected_query"], "SELECT * FROM students;")

    def test_query_logger(self):
        entry = self.logger.log("test prompt", "SELECT * FROM students;", "SQL Query", 12.5, True, 5)
        self.assertEqual(entry["executed_sql"], "SELECT * FROM students;")
        history = self.logger.get_history()
        self.assertGreater(len(history), 0)

if __name__ == "__main__":
    unittest.main()
