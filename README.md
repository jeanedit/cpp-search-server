# Search Server
Implementation of a document catalog search server. Given a search query, it retrieves the most relevant document based on the statistical TF-IDF measure. The server takes into account stop words and negative keywords when searching for documents, handles duplicates, and utilizes multiple threads to process queries. The user receives the top 5 most relevant documents as output.

# Usage
0. Install all necessary components.
1. Initialize the search server using stop words.
2. Add documents to the server.
3. Formulate the query queue.
4. Output the results.
5. Tests will help you explore the capabilities of this search server in more detail.

# System Requirements
1. C++17
2. Any of the following compilers: GCC, MSVC, CLANG.

# Future Enhancements
1. Ability to add documents using a file.
2. A graphical user interface for convenient interaction with the server.
