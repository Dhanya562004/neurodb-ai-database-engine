import re
from typing import Dict, Any, List

class QueryValidator:
    """
    Validates SQL queries before sending them to the C++ backend.
    Detects typos, missing keywords, syntax errors, and provides correction hints.
    """
    KEYWORD_REPLACEMENTS = {
        r"\bSELEKT\b": "SELECT",
        r"\bSELEC\b": "SELECT",
        r"\bFROMM\b": "FROM",
        r"\bFRM\b": "FROM",
        r"\bWHER\b": "WHERE",
        r"\bWHEREER\b": "WHERE",
        r"\bINSERRT\b": "INSERT",
        r"\bINSRT\b": "INSERT",
        r"\bUPDAT\b": "UPDATE",
        r"\bUPDATEE\b": "UPDATE",
        r"\bDELET\b": "DELETE",
        r"\bCREAT\b": "CREATE",
        r"\bVALUS\b": "VALUES",
        r"\bVALUE\b": "VALUES",
    }

    VALID_KEYWORDS = {"SELECT", "INSERT", "CREATE", "UPDATE", "DELETE", "HELP", "EXIT"}

    @classmethod
    def validate(cls, query: str) -> Dict[str, Any]:
        """
        Validates the SQL query.
        Returns dict with 'is_valid', 'suggestion', 'warnings', and 'corrected_query'.
        """
        raw_query = query.strip()
        if not raw_query:
            return {
                "is_valid": False,
                "suggestion": "Query is empty.",
                "warnings": ["Empty query string"],
                "corrected_query": query
            }

        corrected = raw_query
        warnings = []
        has_typo = False

        # Check keyword typos
        for pattern, replacement in cls.KEYWORD_REPLACEMENTS.items():
            if re.search(pattern, corrected, flags=re.IGNORECASE):
                corrected = re.sub(pattern, replacement, corrected, flags=re.IGNORECASE)
                has_typo = True
                warnings.append(f"Auto-corrected keyword typo to '{replacement}'")

        # Check first keyword validity
        words = corrected.split()
        first_word = words[0].upper() if words else ""

        if first_word not in cls.VALID_KEYWORDS:
            # Try fuzzy match for first word
            best_match = None
            for kw in cls.VALID_KEYWORDS:
                if cls._levenshtein_distance(first_word, kw) <= 2:
                    best_match = kw
                    break
            if best_match:
                corrected = best_match + " " + " ".join(words[1:])
                warnings.append(f"First keyword '{first_word}' recognized as '{best_match}'")
            else:
                return {
                    "is_valid": False,
                    "suggestion": f"Unknown SQL keyword '{first_word}'. Expected SELECT, INSERT, CREATE, UPDATE, or DELETE.",
                    "warnings": [f"Unrecognized starting keyword '{first_word}'"],
                    "corrected_query": query
                }

        # Check parenthesis balance
        if corrected.count("(") != corrected.count(")"):
            warnings.append("Mismatched parentheses count.")

        return {
            "is_valid": True,
            "has_corrections": has_typo or len(warnings) > 0,
            "suggestion": f"Did you mean: {corrected}" if (has_typo or len(warnings) > 0) else None,
            "warnings": warnings,
            "corrected_query": corrected
        }

    @staticmethod
    def _levenshtein_distance(s1: str, s2: str) -> int:
        if len(s1) < len(s2):
            return QueryValidator._levenshtein_distance(s2, s1)
        if len(s2) == 0:
            return len(s1)

        previous_row = range(len(s2) + 1)
        for i, c1 in enumerate(s1):
            current_row = [i + 1]
            for j, c2 in enumerate(s2):
                insertions = previous_row[j + 1] + 1
                deletions = current_row[j] + 1
                substitutions = previous_row[j] + (c1 != c2)
                current_row.append(min(insertions, deletions, substitutions))
            previous_row = current_row
        return previous_row[-1]
