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

# Page configuration
st.set_page_config(
    page_title="NeuroDB - AI Database Engine",
    page_icon="⚡",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Load Custom CSS
def load_css():
    css_path = os.path.join(os.path.dirname(__file__), "style.css")
    if os.path.exists(css_path):
        with open(css_path, "r") as f:
            st.markdown(f"<style>{f.read()}</style>", unsafe_allow_html=True)

load_css()

# Initialize Singletons in Session State
if "connector" not in st.session_state:
    st.session_state.connector = NeuroDBConnector()
if "translator" not in st.session_state:
    st.session_state.translator = AITranslator()
if "logger" not in st.session_state:
    st.session_state.logger = QueryLogger()

connector = st.session_state.connector
translator = st.session_state.translator
logger = st.session_state.logger

# Sidebar Header & Schema Inspector
st.sidebar.markdown("""
<div style="text-align: center; padding: 10px 0;">
    <h2 style="color: #6C5CE7; margin-bottom: 0;">⚡ NeuroDB</h2>
    <p style="color: #A0AEC0; font-size: 0.85rem;">AI-Powered File-Based Engine</p>
</div>
""", unsafe_allow_html=True)

st.sidebar.divider()

# Database Schema Explorer
st.sidebar.subheader("📊 Database Schema Explorer")
schema_data = connector.get_schema()

if schema_data:
    for table in schema_data:
        tbl_name = table.get("name", "table")
        col_count = len(table.get("columns", []))
        row_cnt = table.get("row_count", 0)
        
        with st.sidebar.expander(f"📁 {tbl_name} ({row_cnt} rows, {col_count} cols)"):
            cols_df = pd.DataFrame(table.get("columns", []))
            if not cols_df.empty:
                st.dataframe(cols_df, use_container_width=True, hide_index=True)
else:
    st.sidebar.warning("No tables loaded or engine unreachable.")

st.sidebar.divider()

# Sample Queries Section
st.sidebar.subheader("💡 Quick Sample Queries")
sample_queries = [
    ("NL", "show students with marks greater than 80"),
    ("NL", "get employees in IT department"),
    ("SQL", "SELECT * FROM students;"),
    ("SQL", "SELECT * FROM products WHERE price < 100;"),
    ("SQL", "SELECT department, AVG(salary) FROM employees GROUP BY department;")
]

for mode_type, q_text in sample_queries:
    if st.sidebar.button(f"[{mode_type}] {q_text[:30]}...", key=f"btn_{q_text}"):
        st.session_state.current_query = q_text
        st.session_state.query_mode = "Natural Language (AI)" if mode_type == "NL" else "SQL Query"

# Main App Header
st.markdown("""
<div class="hero-container">
    <h1 class="hero-title">NeuroDB AI Database Engine</h1>
    <p class="hero-subtitle">Production-grade C++ Storage & Query Processor with Python Wrapper & Natural Language AI Translation</p>
</div>
""", unsafe_allow_html=True)

# Query Input Mode Selection
query_mode = st.radio(
    "Select Query Input Mode:",
    ["Natural Language (AI)", "SQL Query"],
    horizontal=True,
    key="query_mode"
)

# Text area query input
default_q = st.session_state.get("current_query", "show students with marks greater than 80" if query_mode == "Natural Language (AI)" else "SELECT * FROM students;")
user_input = st.text_area(
    "Enter your query:",
    value=default_q,
    height=100,
    placeholder="e.g. show students with marks greater than 80 or SELECT * FROM students WHERE marks > 80;"
)

col1, col2, col3 = st.columns([1, 2, 1])
with col1:
    execute_btn = st.button("🚀 Execute Query", type="primary", use_container_width=True)

if execute_btn and user_input.strip():
    raw_prompt = user_input.strip()
    target_sql = raw_prompt
    translation_info = None

    # Step 1: Natural Language Translation if selected
    if query_mode == "Natural Language (AI)":
        with st.spinner("🤖 Translating Natural Language to SQL..."):
            translation_info = translator.translate(raw_prompt, schema_data)
            target_sql = translation_info["sql"]

    # Step 2: Query Validation & Hinting
    validation = QueryValidator.validate(target_sql)
    if validation.get("has_corrections") and validation.get("suggestion"):
        st.info(f"💡 **Syntax Hint / Correction**: {validation['suggestion']}")
        target_sql = validation["corrected_query"]

    # Display Generated SQL preview if in NL mode
    if translation_info:
        st.markdown(f"**Generated SQL Query**: `{target_sql}` *(via {translation_info['mode']})*")

    # Step 3: Subprocess Execution on C++ Engine
    with st.spinner("⚡ Executing query on C++ engine..."):
        result = connector.execute_query(target_sql)

    # Step 4: Log query to JSON history
    logger.log(
        raw_query=raw_prompt,
        executed_sql=target_sql,
        mode=query_mode,
        duration_ms=result["execution_time_ms"],
        success=result["success"],
        row_count=result["row_count"],
        error=result.get("error")
    )

    # Step 5: Render Performance Metrics Cards
    mcol1, mcol2, mcol3, mcol4 = st.columns(4)
    with mcol1:
        st.metric("Execution Latency", f"{result['execution_time_ms']:.2f} ms")
    with mcol2:
        st.metric("Rows Returned", result["row_count"])
    with mcol3:
        status_str = "SUCCESS" if result["success"] else "FAILED"
        st.metric("Status", status_str)
    with mcol4:
        st.metric("Backend Engine", "C++ Core (v2.0)")

    # Step 6: Render Tabular Results
    st.markdown("### 📋 Query Results")
    if result["success"]:
        df = result["dataframe"]
        if not df.empty:
            st.dataframe(df, use_container_width=True)
        else:
            st.success("Query executed successfully. No rows returned.")
    else:
        st.error(f"❌ Execution Error: {result['error']}")

# Query History Tab in Sidebar / Lower Accordion
st.divider()
with st.expander("📜 Live Query Logs & Audit Trail"):
    history = logger.get_history()
    if history:
        history_df = pd.DataFrame(history)
        st.dataframe(
            history_df[["timestamp", "raw_query", "executed_sql", "duration_ms", "status", "row_count"]],
            use_container_width=True,
            hide_index=True
        )
    else:
        st.info("No query logs recorded yet.")
