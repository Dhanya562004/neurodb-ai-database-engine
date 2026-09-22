import streamlit as st
import pandas as pd
import sys
import os
import time
import re

# Ensure project root is in python path
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

from python_wrapper import NeuroDBConnector, AITranslator, QueryValidator, QueryLogger

# ------------------ FALLBACK LOADER & EXECUTOR ------------------
def load_tables_fallback():
    tables = {}
    data_path = os.path.join(BASE_DIR, "data")

    if os.path.exists(data_path):
        for root, _, files in os.walk(data_path):
            for file in files:
                if file.endswith(".csv"):
                    name = os.path.splitext(file)[0].lower()
                    file_path = os.path.join(root, file)
                    try:
                        tables[name] = pd.read_csv(file_path)
                    except Exception:
                        pass
    return tables

def get_fallback_schema(tables: dict):
    schema_data = []
    for name, df in tables.items():
        schema_data.append({
            "name": name,
            "columns": [{"name": col, "type": str(df[col].dtype)} for col in df.columns],
            "row_count": len(df)
        })
    return schema_data

def execute_fallback_query(target_sql: str, tables: dict) -> dict:
    sql_clean = target_sql.strip().rstrip(";")
    match = re.search(r"SELECT\s+\*\s+FROM\s+([a-zA-Z0-9_]+)(?:\s+WHERE\s+(.+))?", sql_clean, re.IGNORECASE)

    if not match:
        if "SELECT * FROM" in sql_clean.upper():
            parts = sql_clean.upper().split("FROM")
            if len(parts) > 1:
                tbl_part = parts[1].strip().split()[0].lower()
                if tbl_part in tables:
                    df = tables[tbl_part]
                    return {
                        "success": True,
                        "raw_output": df.to_string(),
                        "dataframe": df,
                        "execution_time_ms": 1.0,
                        "error": None,
                        "row_count": len(df)
                    }

        return {
            "success": False,
            "raw_output": "Unsupported query for fallback mode.",
            "dataframe": pd.DataFrame(),
            "execution_time_ms": 1.0,
            "error": "Only SELECT * FROM table_name queries are supported in cloud fallback mode.",
            "row_count": 0
        }

    table_name = match.group(1).lower()
    where_clause = match.group(2)

    if table_name not in tables:
        return {
            "success": False,
            "raw_output": f"Table '{table_name}' not found.",
            "dataframe": pd.DataFrame(),
            "execution_time_ms": 1.0,
            "error": f"Table '{table_name}' not found.",
            "row_count": 0
        }

    df = tables[table_name].copy()

    if where_clause:
        try:
            w_match = re.search(r"([a-zA-Z0-9_]+)\s*(=|>|<|>=|<=)\s*(.+)", where_clause.strip())
            if w_match:
                col, op, val = w_match.group(1), w_match.group(2), w_match.group(3).strip("'\" ")
                if col in df.columns:
                    if pd.api.types.is_numeric_dtype(df[col]):
                        val = float(val) if "." in val else int(val)
                    if op == "=":
                        df = df[df[col] == val]
                    elif op == ">":
                        df = df[df[col] > val]
                    elif op == "<":
                        df = df[df[col] < val]
                    elif op == ">=":
                        df = df[df[col] >= val]
                    elif op == "<=":
                        df = df[df[col] <= val]
        except Exception:
            pass

    return {
        "success": True,
        "raw_output": df.to_string(),
        "dataframe": df,
        "execution_time_ms": 1.0,
        "error": None,
        "row_count": len(df)
    }

# ------------------ PAGE CONFIG ------------------
st.set_page_config(
    page_title="NeuroDB - AI Database Engine",
    page_icon="⚡",
    layout="wide"
)

# ------------------ INIT ------------------
if "connector" not in st.session_state:
    st.session_state.connector = NeuroDBConnector()
if "translator" not in st.session_state:
    st.session_state.translator = AITranslator()
if "logger" not in st.session_state:
    st.session_state.logger = QueryLogger()

connector = st.session_state.connector
translator = st.session_state.translator
logger = st.session_state.logger

# ------------------ LOAD SCHEMA ------------------
schema_data = []
try:
    schema_data = connector.get_schema()
    if not schema_data:
        raise RuntimeError("C++ engine returned empty schema or engine unreachable")
except Exception:
    tables = load_tables_fallback()
    schema_data = get_fallback_schema(tables)

# ------------------ SIDEBAR ------------------
st.sidebar.title("⚡ NeuroDB")

st.sidebar.subheader("📊 Database Schema Explorer")

if schema_data:
    for table in schema_data:
        with st.sidebar.expander(f"{table['name']} ({table['row_count']} rows)"):
            df = pd.DataFrame(table["columns"])
            st.dataframe(df, use_container_width=True)
else:
    st.sidebar.warning("No tables loaded")

# ------------------ MAIN ------------------
st.title("NeuroDB AI Database Engine")

query_mode = st.radio(
    "Select Mode",
    ["Natural Language (AI)", "SQL Query"]
)

user_input = st.text_area("Enter Query")

if st.button("Execute") and user_input.strip():

    raw_prompt = user_input.strip()
    target_sql = raw_prompt

    # -------- NL TO SQL --------
    if query_mode == "Natural Language (AI)":
        try:
            translation = translator.translate(raw_prompt, schema_data)
            target_sql = translation["sql"]
            st.info(f"Generated SQL: {target_sql}")
        except:
            st.warning("AI translation failed, using raw input")

    # -------- VALIDATION --------
    validation = QueryValidator.validate(target_sql)
    if validation.get("has_corrections"):
        target_sql = validation["corrected_query"]

    # -------- EXECUTION --------
    start = time.time()

    try:
        result = connector.execute_query(target_sql)
        if not result.get("success"):
            err = str(result.get("error", ""))
            if any(term in err.lower() for term in ["not found", "exec format error", "permission denied", "no such file", "unreachable", "cannot find", "oserror"]):
                raise RuntimeError(err)
    except Exception:
        # ---------- FALLBACK EXECUTION ----------
        tables = load_tables_fallback()
        result = execute_fallback_query(target_sql, tables)

    end = time.time()
    result["execution_time_ms"] = round((end - start) * 1000, 2)

    # -------- LOGGING --------
    logger.log(
        raw_query=raw_prompt,
        executed_sql=target_sql,
        mode=query_mode,
        duration_ms=result["execution_time_ms"],
        success=result["success"],
        row_count=result["row_count"],
        error=result.get("error")
    )

    # -------- OUTPUT --------
    st.subheader("Result")

    if result["success"]:
        if not result["dataframe"].empty:
            st.dataframe(result["dataframe"], use_container_width=True)
        else:
            st.success("Query executed. No rows returned.")
    else:
        st.error(result["error"])

# ------------------ HISTORY ------------------
st.divider()

with st.expander("Query Logs"):
    history = logger.get_history()
    if history:
        st.dataframe(pd.DataFrame(history))
    else:
        st.info("No logs yet")