import os
import json
import time
from typing import Dict, Any, List

class QueryLogger:
    """
    Logs executed database queries to logs/query_history.json for auditing,
    analytics, and history display in Streamlit frontend.
    """
    def __init__(self, log_path: str = None):
        if log_path is None:
            base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
            log_dir = os.path.join(base_dir, "logs")
            os.makedirs(log_dir, exist_ok=True)
            log_path = os.path.join(log_dir, "query_history.json")
        
        self.log_path = log_path
        if not os.path.exists(self.log_path):
            self._write_logs([])

    def _read_logs(self) -> List[Dict[str, Any]]:
        try:
            with open(self.log_path, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            return []

    def _write_logs(self, logs: List[Dict[str, Any]]):
        try:
            with open(self.log_path, "w", encoding="utf-8") as f:
                json.dump(logs, f, indent=2)
        except Exception as e:
            pass

    def log(self, raw_query: str, executed_sql: str, mode: str, duration_ms: float, success: bool, row_count: int, error: str = None) -> Dict[str, Any]:
        logs = self._read_logs()
        entry = {
            "id": len(logs) + 1,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "raw_query": raw_query,
            "executed_sql": executed_sql,
            "mode": mode,
            "duration_ms": round(duration_ms, 2),
            "status": "SUCCESS" if success else "FAILED",
            "row_count": row_count,
            "error": error
        }
        logs.insert(0, entry) # Most recent first
        # Keep last 100 entries
        logs = logs[:100]
        self._write_logs(logs)
        return entry

    def get_history(self, limit: int = 50) -> List[Dict[str, Any]]:
        logs = self._read_logs()
        return logs[:limit]

    def clear_history(self):
        self._write_logs([])
