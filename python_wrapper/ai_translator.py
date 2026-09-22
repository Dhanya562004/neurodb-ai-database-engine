import os
import re
import requests
from typing import Dict, Any, List, Optional

class AITranslator:
    """
    Translates Natural Language prompts into executable SQL queries.
    Uses Gemini LLM API when GEMINI_API_KEY is available, with an offline rule-based fallback.
    """
    def __init__(self, api_key: Optional[str] = None):
        self.api_key = api_key or os.environ.get("GEMINI_API_KEY")

    def translate(self, nl_prompt: str, schema_info: Optional[List[Dict[str, Any]]] = None) -> Dict[str, Any]:
        """
        Translates NL prompt to SQL query.
        Returns dict with 'sql', 'mode' ('AI' or 'Rule-Based'), and 'explanation'.
        """
        nl_prompt_clean = nl_prompt.strip()

        # Try Gemini API if key is present
        if self.api_key:
            try:
                sql = self._call_gemini_api(nl_prompt_clean, schema_info)
                if sql:
                    return {
                        "sql": sql,
                        "mode": "Gemini AI",
                        "explanation": f"Translated using Google Gemini LLM API."
                    }
            except Exception as e:
                pass

        # Fallback to rule-based parser
        sql = self._rule_based_translate(nl_prompt_clean, schema_info)
        return {
            "sql": sql,
            "mode": "Rule-Based Engine",
            "explanation": "Translated using offline pattern-matching fallback engine."
        }

    def _call_gemini_api(self, prompt: str, schema_info: Optional[List[Dict[str, Any]]]) -> Optional[str]:
        schema_text = ""
        if schema_info:
            schema_text = "Database Schema:\n" + "\n".join(
                [f"Table {t['name']}: " + ", ".join([f"{c['name']} ({c['type']})" for c in t['columns']])
                 for t in schema_info]
            )

        system_instruction = (
            "You are a SQL query generator for a C++ SQL database engine. "
            "Convert the user's natural language request into a single valid SQL query. "
            "Do NOT include markdown block syntax like ```sql. Output ONLY the raw SQL query string ending with a semicolon. "
            f"{schema_text}"
        )

        url = f"https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent?key={self.api_key}"
        payload = {
            "contents": [
                {
                    "parts": [
                        {"text": f"{system_instruction}\nUser request: {prompt}"}
                    ]
                }
            ]
        }
        res = requests.post(url, json=payload, timeout=5)
        if res.status_code == 200:
            data = res.json()
            raw_text = data['candidates'][0]['content']['parts'][0]['text'].strip()
            # Remove any markdown formatting
            raw_text = re.sub(r"^```sql\s*", "", raw_text, flags=re.IGNORECASE)
            raw_text = re.sub(r"^```\s*", "", raw_text)
            raw_text = re.sub(r"\s*```$", "", raw_text).strip()
            if not raw_text.endswith(";"):
                raw_text += ";"
            return raw_text
        return None

    def _rule_based_translate(self, prompt: str, schema_info: Optional[List[Dict[str, Any]]]) -> str:
        text = prompt.lower().strip()
        if text.endswith("."):
            text = text[:-1].strip()

        # Extract table name if present in prompt
        known_tables = ["students", "users", "products", "employees", "orders"]
        if schema_info:
            known_tables = [t['name'].lower() for t in schema_info]

        target_table = None
        for tbl in known_tables:
            if tbl in text or tbl[:-1] in text: # e.g. student -> students
                target_table = tbl
                break
        
        if not target_table:
            target_table = "students" # default sample table

        # Pattern 1: "show/get/find <table/cols> with/where/having <column> <operator> <value>"
        # e.g., "show students with marks greater than 80"
        gt_match = re.search(r"(?:greater than|more than|above|>)\s*(\d+(?:\.\d+)?)", text)
        lt_match = re.search(r"(?:less than|below|under|<)\s*(\d+(?:\.\d+)?)", text)
        eq_match = re.search(r"(?:equals?|equal to|is|=)\s*['\"]?([a-zA-Z0-9_\-\s@.]+?)['\"]?$", text)

        # Detect column name in condition
        col_name = "marks" if "mark" in text else ("gpa" if "gpa" in text else ("age" if "age" in text else ("price" if "price" in text else ("salary" if "salary" in text else "id"))))

        if gt_match:
            val = gt_match.group(1)
            return f"SELECT * FROM {target_table} WHERE {col_name} > {val};"

        if lt_match:
            val = lt_match.group(1)
            return f"SELECT * FROM {target_table} WHERE {col_name} < {val};"

        if "in" in text and ("department" in text or "category" in text or "major" in text):
            match = re.search(r"(?:department|category|major)\s+(?:of\s+)?['\"]?([a-zA-Z0-9_\s]+)['\"]?", text)
            if match:
                val = match.group(1).strip()
                col = "department" if "department" in text else ("category" if "category" in text else "major")
                return f"SELECT * FROM {target_table} WHERE {col} = '{val}';"

        if "count" in text:
            return f"SELECT * FROM {target_table};"

        # Default fallback query
        return f"SELECT * FROM {target_table};"
