# ⚡ NeuroDB: AI-Powered File-Based Database Engine

[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10+-yellow.svg)](https://www.python.org/)
[![Streamlit](https://img.shields.io/badge/Streamlit-1.30+-FF4B4B.svg)](https://streamlit.io/)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**NeuroDB** is a production-style, hybrid database system that combines a high-performance **C++ engine** (query parsing, AST execution, file-based persistence, and indexing) with an **AI-driven Python interface layer** and an interactive **Streamlit frontend**.

It supports standard SQL commands (`CREATE`, `INSERT`, `SELECT`, `UPDATE`, `DELETE`, `GROUP BY`, `HAVING`), Natural Language query translation via Google Gemini AI (with a robust offline rule-based fallback parser), query validation, auto-correction hints, latency tracking, query execution logging, and an **automatic Python/Pandas fallback engine** for cloud deployments (e.g., Streamlit Cloud).

---

## 🏗️ Architecture Design

NeuroDB is structured as a three-tier hybrid architecture:

```
┌─────────────────────────────────────────────────────────────────┐
│                     Streamlit Web Frontend                      │
│     Natural Language / SQL Query Box | Data Tables | Latency    │
└────────────────────────────────┬────────────────────────────────┘
                                 │ Subprocess / API Inter-Process Call
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                     Python Interface Layer                      │
│  ┌────────────────────┐ ┌──────────────────┐ ┌────────────────┐ │
│  │  AITranslator      │ │ QueryValidator   │ │ QueryLogger    │ │
│  │ (Gemini/Fallback)  │ │ (Typo Corrector) │ │ (JSON Auditor) │ │
│  └────────────────────┘ └──────────────────┘ └────────────────┘ │
└────────────────────────────────┬────────────────────────────────┘
                                 │ High-Speed CLI / IPC Subprocess
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                  Hybrid Query Execution Core                    │
│  ┌──────────────────────────────┐ ┌───────────────────────────┐ │
│  │  C++ Core Database Engine    │ │ Python/Pandas Fallback    │ │
│  │  (Primary Local Engine)      │ │ (Streamlit Cloud Fallback)│ │
│  └──────────────────────────────┘ └───────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## ✨ Features

- **Hybrid C++/Python System**: High-speed C++ engine core executed via a Python `subprocess` connector with sub-millisecond inter-process latency.
- **Automatic Cloud Fallback Engine**: Seamlessly falls back to an in-memory Pandas engine when running on Linux containers / Streamlit Cloud where native binaries differ. Loads schema and executes `SELECT *` queries smoothly without UI or architectural changes.
- **Natural Language to SQL (AI Integration)**: Convert plain English prompts into valid SQL.
  - *Example*: `"show students with marks greater than 80"` $\rightarrow$ `SELECT * FROM students WHERE marks > 80;`
  - Uses **Google Gemini API** when online and falls back to a **Pattern-Matching Regex Engine** offline.
- **Interactive Streamlit Web Dashboard**:
  - Dual query mode (Natural Language AI mode vs Raw SQL mode).
  - Instant query execution latency metric card ($ms$).
  - Tabular DataFrame result rendering.
  - Database schema inspector accordion (view table structures, column data types, row counts).
- **Query Validation & Syntax Correction**: Detects typos (e.g. `SELEKT` $\rightarrow$ `SELECT`) and suggests syntax fixes before execution.
- **Query Execution Logging**: Maintains audit trails of queries, execution timestamps, duration, and status in `logs/query_history.json`.
- **File-Based Persistence**: Table metadata (`.meta`) and structured data (`.csv`) automatically loaded on engine startup and saved on mutation.

---

## 📁 Repository Structure

```
NeuroDB/
├── include/                     # C++ Engine Header Definitions & Parsers
│   ├── models.cpp               # Data models (Value, Variant, Row, Table, Catalog, AST)
│   ├── Helper.cpp               # File utilities, directory loaders, and string parsers
│   ├── CreateParse.cpp          # CREATE TABLE parser
│   ├── InsertParser.cpp         # INSERT INTO parser
│   ├── SelectParser.cpp         # SELECT, WHERE, GROUP BY, HAVING parser
│   ├── UpdateParser.cpp         # UPDATE parser
│   └── DeleteParser.cpp         # DELETE parser
├── src/                         # C++ Backend Entrypoints
│   ├── main.cpp                 # C++ Database CLI & API flags (--query, --schema)
│   └── setup_test_data.cpp      # Seeding script for sample tables & records
├── python_wrapper/              # Python Interface Package
│   ├── __init__.py
│   ├── neurodb_connector.py     # Subprocess wrapper connecting Python to C++ binary
│   ├── ai_translator.py         # NL-to-SQL translator (Gemini API + Rule-based fallback)
│   ├── query_validator.py       # Syntax validator and fuzzy typo corrector
│   └── query_logger.py          # JSON log audit manager
├── frontend/                    # Streamlit Frontend Web App
│   ├── app.py                   # Streamlit dashboard (with automatic C++ / Pandas fallback)
│   └── style.css                # Custom glassmorphic CSS theme
├── data/                        # CSV & Metadata storage directory
├── logs/                        # Persistent Query Execution Logs
│   └── query_history.json
├── scripts/                     # Build Scripts
│   └── build.ps1                # PowerShell automated build script
├── tests/                       # Unit Test Suite
│   └── test_neurodb.py          # Integration & unit tests
├── CMakeLists.txt               # Cross-platform C++ build configuration
├── requirements.txt             # Python dependencies
└── README.md                    # Project Documentation
```

---

## 🚀 Quick Start Guide

### Prerequisites
- **C++ Compiler**: `g++` (supporting C++17) or MinGW / Clang / MSVC.
- **Python**: 3.10+ installed.

### 1. Build the C++ Engine & Seed Database
#### On Windows (PowerShell):
```powershell
# Run the automated build script
.\scripts\build.ps1
```

#### On Linux / macOS / Manual Build:
```bash
# Compile using g++
g++ -std=c++17 -Iinclude src/main.cpp -o neurodb
g++ -std=c++17 -Iinclude src/setup_test_data.cpp -o setup_test_data

# Seed initial test data
./setup_test_data
```

### 2. Install Python Dependencies
```bash
pip install -r requirements.txt
```

### 3. Launch the Streamlit Frontend Web App
```bash
streamlit run frontend/app.py
```
Open your browser at `http://localhost:8501` to use the interactive AI database engine!

### 4. Run Automated Unit Tests
```bash
python -m unittest discover -s tests
```

---

## 🌐 Deploying to Streamlit Cloud (Live URL)

To share your live web application online:

1. **Push Code to GitHub**:
   Ensure your repository is pushed to GitHub (`https://github.com/Dhanya562004/neurodb-ai-database-engine.git`).
2. **Connect Streamlit Community Cloud**:
   - Go to [share.streamlit.io](https://share.streamlit.io/) and log in with your GitHub account.
   - Click **New app**.
   - Select repository: `Dhanya562004/neurodb-ai-database-engine`.
   - Main file path: `frontend/app.py`.
3. **Environment Variables (Optional)**:
   - In App Settings $\rightarrow$ Secrets, add:
     ```toml
     GEMINI_API_KEY = "your_google_gemini_api_key"
     ```
4. **Deploy**:
   - Click **Deploy!** Your app will build and publish a live public URL. (The built-in fallback system will automatically power schema loading and query execution seamlessly on Streamlit Cloud).

---

## 📝 Resume Bullet Points (Software Engineer Role)

- **Engineered a Hybrid AI Database System**: Architected a production-grade multi-tier database engine integrating a C++ core execution backend with a Python interface layer and Streamlit web UI, featuring sub-millisecond IPC query latency and automatic cloud fallback.
- **LLM-Powered Query Processing**: Integrated Google Gemini API with an offline pattern-matching fallback parser to translate natural language prompts into executable SQL queries, featuring automated syntax validation and typo auto-correction.
- **File-Based Storage & Query Performance**: Implemented file-based persistence for relational tables (`.meta`/`.csv`), Primary Key index hash lookups, aggregate functions (`GROUP BY`/`HAVING`), and persistent query execution audit logging.

---

## 📄 License
This project is licensed under the [MIT License](LICENSE).