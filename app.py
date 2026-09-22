import streamlit as st
import pandas as pd
import sys
import os
import time

# Ensure project root is in python path
BASE_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)

from python_wrapper import NeuroDBConnector, AITranslator, QueryValidator, QueryLogger

# ------------------ FALLBACK LOADER ------------------
def load_tables_fallback():
    tables = {}
    data_path = os.path.join(BASE_DIR, "data")

    if os.path.exists(data_path):
        for file in os.listdir(data_path):
            if file.endswith(".csv"):
                name = file.replace(".csv", "").lower()
                tables[name] = pd.read_csv(os.path.join(data_path, file))
    return tables

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
schema_data = connector.get_schema()

# FALLBACK if C++ engine fails
if not schema_data:
    tables = load_tables_fallback()
    schema_data = []

    for name, df in tables.items():
        schema_data.append({
            "name": name,
            "columns": [{"name": col, "type": str(df[col].dtype)} for col in df.columns],
            "row_count": len(df)
        })

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

    except:
        # ---------- FALLBACK EXECUTION ----------
        tables = load_tables_fallback()

        result = {
            "success": True,
            "execution_time_ms": 1,
            "row_count": 0,
            "dataframe": pd.DataFrame()
        }

        try:
            sql = target_sql.upper()

            if "SELECT * FROM" in sql:
                table_name = sql.split("FROM")[1].strip().replace(";", "").lower()

                if table_name in tables:
                    df = tables[table_name]
                    result["dataframe"] = df
                    result["row_count"] = len(df)
                else:
                    result["success"] = False
                    result["error"] = "Table not found"

            else:
                result["success"] = False
                result["error"] = "Only SELECT * supported in cloud mode"

        except Exception as e:
            result["success"] = False
            result["error"] = str(e)

    end = time.time()
    result["execution_time_ms"] = (end - start) * 1000

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