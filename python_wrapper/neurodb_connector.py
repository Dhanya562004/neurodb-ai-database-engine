import subprocess
import os
import json
import time
import pandas as pd
from typing import Dict, Any, List

class NeuroDBConnector:
    """
    Python wrapper interface for the C++ NeuroDB database engine.
    Communicates via subprocess CLI execution.
    """
    def __init__(self, binary_path: str = None):
        self.base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        if binary_path is None:
            executable = "neurodb.exe" if os.name == "nt" else "neurodb"
            binary_path = os.path.join(self.base_dir, executable)
            if not os.path.exists(binary_path):
                binary_path = executable

        self.binary_path = binary_path
        self.env = self._prepare_environment()

    def _prepare_environment(self) -> Dict[str, str]:
        env = dict(os.environ)
        if os.name == "nt":
            mingw_paths = [
                r"C:\Program Files\CodeBlocks\MinGW\bin",
                r"C:\msys64\ucrt64\bin",
                r"C:\msys64\mingw64\bin",
                r"C:\MinGW\bin"
            ]
            current_path = env.get("PATH", "")
            for p in mingw_paths:
                if os.path.exists(p) and p not in current_path:
                    current_path = p + ";" + current_path
            env["PATH"] = current_path
        return env

    def get_schema(self) -> List[Dict[str, Any]]:
        """Fetch database tables and column definitions in JSON format."""
        try:
            res = subprocess.run(
                [self.binary_path, "--schema"],
                cwd=self.base_dir,
                capture_output=True,
                text=True,
                env=self.env,
                check=True
            )
            return json.loads(res.stdout)
        except Exception as e:
            return []

    def execute_query(self, sql_query: str) -> Dict[str, Any]:
        """
        Executes a SQL query on the C++ engine, measures latency,
        and parses output into structured DataFrame / results dictionary.
        """
        start_time = time.perf_counter()
        sql_query = sql_query.strip()
        if not sql_query.endswith(";"):
            sql_query += ";"

        try:
            res = subprocess.run(
                [self.binary_path, "-q", sql_query],
                cwd=self.base_dir,
                capture_output=True,
                text=True,
                env=self.env,
                timeout=10
            )
            duration_ms = (time.perf_counter() - start_time) * 1000.0

            if res.returncode != 0:
                error_msg = res.stdout.strip() or res.stderr.strip() or "Query execution failed."
                return {
                    "success": False,
                    "raw_output": error_msg,
                    "dataframe": pd.DataFrame(),
                    "execution_time_ms": round(duration_ms, 2),
                    "error": error_msg,
                    "row_count": 0
                }

            stdout_text = res.stdout.strip()
            df, row_count = self._parse_output_to_df(stdout_text)

            return {
                "success": True,
                "raw_output": stdout_text,
                "dataframe": df,
                "execution_time_ms": round(duration_ms, 2),
                "error": None,
                "row_count": row_count
            }

        except subprocess.TimeoutExpired:
            return {
                "success": False,
                "raw_output": "Execution timed out.",
                "dataframe": pd.DataFrame(),
                "execution_time_ms": 10000.0,
                "error": "Query execution timed out.",
                "row_count": 0
            }
        except Exception as e:
            duration_ms = (time.perf_counter() - start_time) * 1000.0
            return {
                "success": False,
                "raw_output": str(e),
                "dataframe": pd.DataFrame(),
                "execution_time_ms": round(duration_ms, 2),
                "error": str(e),
                "row_count": 0
            }

    def _parse_output_to_df(self, stdout: str) -> (pd.DataFrame, int):
        lines = [line.strip() for line in stdout.splitlines() if line.strip()]
        if not lines:
            return pd.DataFrame(), 0

        data_lines = []
        row_count = 0
        for line in lines:
            if "row(s) returned" in line:
                try:
                    row_count = int(line.split()[0])
                except:
                    pass
                continue
            data_lines.append(line)

        if not data_lines:
            return pd.DataFrame(), 0

        if "|" in data_lines[0]:
            columns = [c.strip() for c in data_lines[0].split("|")]
            rows = []
            for l in data_lines[1:]:
                if "|" in l:
                    vals = [v.strip() for v in l.split("|")]
                    if len(vals) == len(columns):
                        rows.append(vals)
            df = pd.DataFrame(rows, columns=columns)
            if row_count == 0:
                row_count = len(df)
            return df, row_count
        else:
            return pd.DataFrame({"Result": data_lines}), 1
