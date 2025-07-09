#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <cctype>
#include <clocale>
#include <codecvt>
#include <locale>

using namespace std;

// --------------------------- Livro ----------------------------
struct Livro {
    int codigo;
    string titulo;
    string autor;
    string editora;
    int ano;
    int exemplares;
    string classificacao;
    string campus;

    Livro(int c, const string& t, const string& a, const string& e, int y, int ex, const string& cl, const string& cp)
        : codigo(c), titulo(t), autor(a), editora(e), ano(y), exemplares(ex), classificacao(cl), campus(cp) {}

    void print() const {
        cout << "Cod: " << codigo << endl;
        cout << "Tit: " << titulo << endl;
        cout << "Aut: " << (autor.empty() ? "Desconhecido" : autor) << endl;
        cout << "Edi: " << (editora.empty() ? "Desconhecida" : editora) << endl;
        cout << "Ano: " << (ano == 0 ? "Desconhecido" : to_string(ano)) << endl;
        cout << "Exem: " << exemplares << endl;
        cout << "Cla: " << classificacao << endl;
        cout << "Cam: " << campus << endl << endl;
    }
};

// --------------------- Nó da B+Tree --------------------------
const int ORDEM = 3;

struct BPlusNode {
    bool folha;
    vector<string> chaves;
    vector<BPlusNode*> filhos; // Se !folha
    vector<vector<Livro*>> valores; // Se folha: ponteiros para livros
    BPlusNode* proxFolha;

    BPlusNode(bool isFolha) : folha(isFolha), proxFolha(nullptr) {}
};

// ---------------------- B+Tree completa ------------------------
class BPlusTree {
private:
    BPlusNode* raiz;

    vector<string> dividirChaves(vector<string>& v, int pos) {
        vector<string> res(v.begin() + pos, v.end());
        v.resize(pos);
        return res;
    }

    vector<vector<Livro*>> dividirValores(vector<vector<Livro*>>& v, int pos) {
        vector<vector<Livro*>> res(v.begin() + pos, v.end());
        v.resize(pos);
        return res;
    }

    vector<BPlusNode*> dividirFilhos(vector<BPlusNode*>& v, int pos) {
        vector<BPlusNode*> res(v.begin() + pos, v.end());
        v.resize(pos);
        return res;
    }

    void inserirInterno(BPlusNode* node, const string& chave, Livro* livro) {
        if (node->folha) {
            auto it = lower_bound(node->chaves.begin(), node->chaves.end(), chave);
            int idx = distance(node->chaves.begin(), it);
            if (it != node->chaves.end() && *it == chave) {
                node->valores[idx].push_back(livro);
            } else {
                node->chaves.insert(it, chave);
                node->valores.insert(node->valores.begin() + idx, {livro});
            }
        } else {
            auto it = upper_bound(node->chaves.begin(), node->chaves.end(), chave);
            int idx = distance(node->chaves.begin(), it);
            inserirInterno(node->filhos[idx], chave, livro);

            if (node->filhos[idx]->chaves.size() >= ORDEM) {
                dividirNo(node, idx);
            }
        }
    }

    void dividirNo(BPlusNode* pai, int idx) {
        BPlusNode* filho = pai->filhos[idx];
        BPlusNode* novo = new BPlusNode(filho->folha);

        int mid = ORDEM / 2;
        string chavePromovida = filho->chaves[mid];

        novo->chaves = dividirChaves(filho->chaves, filho->folha ? mid : mid + 1);

        if (filho->folha) {
            novo->valores = dividirValores(filho->valores, mid);
            novo->proxFolha = filho->proxFolha;
            filho->proxFolha = novo;
        } else {
            novo->filhos = dividirFilhos(filho->filhos, mid + 1);
        }

        pai->chaves.insert(pai->chaves.begin() + idx, chavePromovida);
        pai->filhos.insert(pai->filhos.begin() + idx + 1, novo);
    }

public:
    BPlusTree() {
        raiz = new BPlusNode(true);
    }

    void inserir(const string& chave, Livro* livro) {
        inserirInterno(raiz, chave, livro);

        if (raiz->chaves.size() >= ORDEM) {
            BPlusNode* novaRaiz = new BPlusNode(false);
            novaRaiz->filhos.push_back(raiz);
            dividirNo(novaRaiz, 0);
            raiz = novaRaiz;
        }
    }

    vector<Livro*> buscarPrefixo(const string& prefixo) {
        vector<Livro*> resultado;
        BPlusNode* node = raiz;

        while (!node->folha) {
            auto it = upper_bound(node->chaves.begin(), node->chaves.end(), prefixo);
            int idx = distance(node->chaves.begin(), it);
            node = node->filhos[idx];
        }

        while (node) {
            for (size_t i = 0; i < node->chaves.size(); ++i) {
                if (node->chaves[i].compare(0, prefixo.size(), prefixo) == 0) {
                    resultado.insert(resultado.end(), node->valores[i].begin(), node->valores[i].end());
                } else if (node->chaves[i] > prefixo) {
                    return resultado;
                }
            }
            node = node->proxFolha;
        }
        return resultado;
    }
        bool removerInterno(BPlusNode* node, const string& chave, Livro* livro) {
        if (!node->folha) {
            for (size_t i = 0; i < node->chaves.size(); ++i) {
                if (chave <= node->chaves[i]) {
                    return removerInterno(node->filhos[i], chave, livro);
                }
            }
            return removerInterno(node->filhos.back(), chave, livro);
        } else {
            for (size_t i = 0; i < node->chaves.size(); ++i) {
                if (node->chaves[i] == chave) {
                    auto& lista = node->valores[i];
                    auto it = find(lista.begin(), lista.end(), livro);
                    if (it != lista.end()) {
                        lista.erase(it);
                        if (lista.empty()) {
                            node->chaves.erase(node->chaves.begin() + i);
                            node->valores.erase(node->valores.begin() + i);
                        }
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool remover(const string& chave, Livro* livro) {
        return removerInterno(raiz, chave, livro);
    }

};

// ---------------------- Utilitários ------------------------
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

vector<string> extrairPalavras(const string& texto) {
    vector<string> palavras;
    stringstream ss(texto);
    string p;
    while (ss >> p) {
        transform(p.begin(), p.end(), p.begin(), [](unsigned char c){ return tolower(c); });
        palavras.push_back(p);
    }
    return palavras;
}

vector<string> parseCSVLine(const string& line) {
    vector<string> fields;
    bool inQuotes = false;
    string currentField;

    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(trim(currentField));
            currentField.clear();
        } else {
            currentField += c;
        }
    }
    fields.push_back(trim(currentField));
    return fields;
}

int extrairAno(const string& data) {
    size_t pos = data.find('-');
    if (pos != string::npos) {
        string anoStr = data.substr(0, pos);
        try {
            return stoi(anoStr);
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

string extrairAutor(const string& entradaPrincipal) {
    if (entradaPrincipal.empty() || entradaPrincipal == " ") return "";
    
    size_t start = entradaPrincipal.find_first_not_of(" ");
    if (start == string::npos) return "";
    
    size_t end = entradaPrincipal.find_first_of(",;", start);
    if (end == string::npos) {
        end = entradaPrincipal.length();
    }
    
    return entradaPrincipal.substr(start, end - start);
}
void removerLivro(BPlusTree& indiceTitulo, BPlusTree& indiceAutor,
                  unordered_map<int, Livro*>& indiceCodigo, vector<Livro*>& livros,
                  const string& caminhoCSV) {
    cout << "Digite o código do livro a remover: ";
    int codigo;
    cin >> codigo;
    cin.ignore();

    if (!indiceCodigo.count(codigo)) {
        cout << "Livro não encontrado!\n";
        return;
    }

    Livro* livro = indiceCodigo[codigo];

    // Remove dos índices
    for (const auto& palavra : extrairPalavras(livro->titulo)) {
        indiceTitulo.remover(palavra, livro);
    }
    for (const auto& palavra : extrairPalavras(livro->autor)) {
        indiceAutor.remover(palavra, livro);
    }

    // Remove do vetor e do mapa
    indiceCodigo.erase(codigo);
    livros.erase(remove(livros.begin(), livros.end(), livro), livros.end());
    delete livro;

    // Reescreve todo o CSV com os livros restantes
    ofstream out(caminhoCSV);
    if (!out.is_open()) {
        cout << "Erro ao abrir o arquivo para reescrita!\n";
        return;
    }

    // Cabeçalho (ajuste conforme o original do seu CSV)
    out << "Código,Autor,Título,,,,,,,,Classificação,Ano,,,,,,Exemplares,,,,Campus,,,Editora\n";

    for (const auto* l : livros) {
        out << l->codigo << ",\"" << l->autor << "\",\"" << l->titulo << "\",,,,,,,," 
            << l->classificacao << "," << l->ano << ",,,,," 
            << l->exemplares << ",,,," 
            << l->campus << ",,,\"" << l->editora << "\"\n";
    }

    out.close();

    cout << "Livro removido com sucesso!\n";
}

void adicionarLivro(BPlusTree& indiceTitulo, BPlusTree& indiceAutor,
                    unordered_map<int, Livro*>& indiceCodigo, vector<Livro*>& livros,
                    const string& caminhoCSV) {
    int codigo, ano, exemplares;
    string titulo, autor, editora, classificacao, campus;

    cout << "Código: "; cin >> codigo; cin.ignore();
    cout << "Título: "; getline(cin, titulo);
    cout << "Autor: "; getline(cin, autor);
    cout << "Editora: "; getline(cin, editora);
    cout << "Ano: "; cin >> ano; cin.ignore();
    cout << "Exemplares: "; cin >> exemplares; cin.ignore();
    cout << "Classificação: "; getline(cin, classificacao);
    cout << "Campus: "; getline(cin, campus);

    Livro* livro = new Livro(codigo, titulo, autor, editora, ano, exemplares, classificacao, campus);
    livros.push_back(livro);
    indiceCodigo[codigo] = livro;

    for (const auto& palavra : extrairPalavras(titulo)) {
        indiceTitulo.inserir(palavra, livro);
    }

    if (!autor.empty()) {
        for (const auto& palavra : extrairPalavras(autor)) {
            indiceAutor.inserir(palavra, livro);
        }
    }

    // Adiciona no CSV
    ofstream out(caminhoCSV, ios::app);
    out << codigo << ",\"" << autor << "\",\"" << titulo << "\",,,,,,,," 
        << classificacao << "," << ano << ",,,,," 
        << exemplares << ",,,," 
        << campus << ",,,\"" << editora << "\"\n";
    out.close();

    cout << "Livro adicionado com sucesso!\n";
}


// ------------------------ Main ----------------------------
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Portuguese_Brazil.1252");

    BPlusTree indiceTitulo;
    BPlusTree indiceAutor;
    unordered_map<int, Livro*> indiceCodigo;
    vector<Livro*> livros;

    if (argc < 2) {
        cerr << "Uso: " << argv[0] << " <arquivo_csv> [comando] [parametros...]" << endl;
        return 1;
    }

    // --------- Carregar acervo do CSV ---------
    ifstream arquivo(argv[1]);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << argv[1] << endl;
        return 1;
    }

    string linha;
    getline(arquivo, linha); // cabeçalho

    while (getline(arquivo, linha)) {
        vector<string> campos = parseCSVLine(linha);
        if (campos.size() < 25) continue;

        int codigo = 0;
        try { codigo = stoi(campos[0]); } catch (...) { continue; }

        string titulo = campos[2];
        string autor = extrairAutor(campos[1]);
        string editora = campos[24];
        int ano = 0;
        try { ano = stoi(campos[11]); } catch (...) {}
        string classificacao = campos[10];
        string campus = campos[20];
        int exemplares = 1;
        try { exemplares = stoi(campos[15]); } catch (...) {}

        Livro* livro = new Livro(codigo, titulo, autor, editora, ano, exemplares, classificacao, campus);
        livros.push_back(livro);
        indiceCodigo[codigo] = livro;

        for (const auto& palavra : extrairPalavras(titulo))
            indiceTitulo.inserir(palavra, livro);

        if (!autor.empty()) {
            for (const auto& palavra : extrairPalavras(autor))
                indiceAutor.inserir(palavra, livro);
        }
    }
    arquivo.close();

    // --------- Comandos via linha de comando ---------
    if (argc >= 3) {
        string comando = argv[2];

        if (comando == "add" && argc == 11) {
            string titulo = argv[3];
            string autor = argv[4];
            string editora = argv[5];
            int ano = stoi(argv[6]);
            int exemplares = stoi(argv[7]);
            string classificacao = argv[8];
            string campus = argv[9];
            int codigo = stoi(argv[10]);

            Livro* livro = new Livro(codigo, titulo, autor, editora, ano, exemplares, classificacao, campus);
            livros.push_back(livro);
            indiceCodigo[codigo] = livro;

            for (const auto& palavra : extrairPalavras(titulo))
                indiceTitulo.inserir(palavra, livro);
            if (!autor.empty()) {
                for (const auto& palavra : extrairPalavras(autor))
                    indiceAutor.inserir(palavra, livro);
            }

            ofstream out(argv[1], ios::app);
            out << codigo << ",\"" << autor << "\",\"" << titulo << "\",,,,,,,," 
                << classificacao << "," << ano << ",,,,," 
                << exemplares << ",,,," 
                << campus << ",,,\"" << editora << "\"\n";
            out.close();

            cout << "Livro adicionado com sucesso!\n";
            return 0;
        }

        else if (comando == "remove" && argc == 4) {
            int codigo = stoi(argv[3]);

            if (!indiceCodigo.count(codigo)) {
                cout << "Livro não encontrado!\n";
                return 1;
            }

            Livro* livro = indiceCodigo[codigo];
            for (const auto& palavra : extrairPalavras(livro->titulo))
                indiceTitulo.remover(palavra, livro);
            for (const auto& palavra : extrairPalavras(livro->autor))
                indiceAutor.remover(palavra, livro);

            indiceCodigo.erase(codigo);
            livros.erase(remove(livros.begin(), livros.end(), livro), livros.end());
            delete livro;

            ifstream in(argv[1]);
            ofstream out(argv[1] + string(".tmp"));
            if (!in.is_open() || !out.is_open()) {
                cout << "Erro ao acessar arquivo CSV para remoção.\n";
                return 1;
            }

            string linha;
            bool primeiraLinha = true;
            while (getline(in, linha)) {
                if (primeiraLinha) {
                    out << linha << "\n";
                    primeiraLinha = false;
                    continue;
                }
                vector<string> campos = parseCSVLine(linha);
                if (campos.size() > 0) {
                    try {
                        int cod = stoi(campos[0]);
                        if (cod == codigo) continue;
                    } catch (...) {}
                }
                out << linha << "\n";
            }
            in.close();
            out.close();
            remove(argv[1]);
            rename((string(argv[1]) + ".tmp").c_str(), argv[1]);

            cout << "Livro removido com sucesso!\n";
            return 0;
        }

        else if (argc == 4) {
            string tipo_busca = comando;
            string query = argv[3];
            transform(query.begin(), query.end(), query.begin(), ::tolower);

            if (tipo_busca == "titulo") {
                vector<Livro*> resultado = indiceTitulo.buscarPrefixo(query);
                for (const auto* l : resultado) l->print();
            } else if (tipo_busca == "autor") {
                vector<Livro*> resultado = indiceAutor.buscarPrefixo(query);
                for (const auto* l : resultado) l->print();
            } else if (tipo_busca == "codigo") {
                int cod = stoi(query);
                if (indiceCodigo.count(cod)) indiceCodigo[cod]->print();
                else cout << "Livro não encontrado!\n";
            } else {
                cerr << "Tipo de busca inválido!\n";
                return 1;
            }
            return 0;
        }

        else {
            cerr << "Uso inválido. Consulte a documentação.\n";
            return 1;
        }
    }

    // --------- Menu interativo padrão ---------
    cout << "\n+----------------------------------------+" << endl;
    cout << "|      Sistema de Busca de Acervo        |" << endl;
    cout << "+----------------------------------------+" << endl;

    while (true) {
        cout << "\n+----------------------------------------+" << endl;
        cout << "|               Opções de Busca          |" << endl;
        cout << "+----------------------------------------+" << endl;
        cout << "| 1. Buscar por palavra no título        |" << endl;
        cout << "| 2. Buscar por autor                    |" << endl;
        cout << "| 3. Buscar por código                   |" << endl;
        cout << "| 4. Adicionar novo livro                |" << endl;
        cout << "| 5. Remover livro por código            |" << endl;
        cout << "| 6. Sair                                |" << endl;
        cout << "+----------------------------------------+" << endl;
        cout << "Escolha uma opção: ";

        string entrada;
        getline(cin, entrada);
        int opcao;
        try {
            opcao = stoi(entrada);
        } catch (...) {
            opcao = -1; // Entrada inválida
        }       

        if (opcao == 6) break;

        switch (opcao) {
            case 1: {
                cout << "Digite uma palavra para buscar no título: ";
                string palavra; getline(cin, palavra);
                transform(palavra.begin(), palavra.end(), palavra.begin(), ::tolower);
                vector<Livro*> resultado = indiceTitulo.buscarPrefixo(palavra);
                for (const auto* l : resultado) l->print();
                break;
            }
            case 2: {
                cout << "Digite uma palavra para buscar no autor: ";
                string palavra; getline(cin, palavra);
                transform(palavra.begin(), palavra.end(), palavra.begin(), ::tolower);
                vector<Livro*> resultado = indiceAutor.buscarPrefixo(palavra);
                for (const auto* l : resultado) l->print();
                break;
            }
            case 3: {
                cout << "Digite o código do livro: ";
                int cod; cin >> cod;
                if (indiceCodigo.count(cod)) indiceCodigo[cod]->print();
                else cout << "Livro não encontrado!\n";
                break;
            }
            case 4:
                adicionarLivro(indiceTitulo, indiceAutor, indiceCodigo, livros, argv[1]);
                break;
            case 5:
                removerLivro(indiceTitulo, indiceAutor, indiceCodigo, livros, argv[1]);
                break;
            default:
                cout << "Opção inválida!\n";
        }
    }

    for (auto* l : livros) delete l;
    return 0;
}


// Compilação: g++ -o main.exe main.cpp
// Execução: ./main.exe ACERVO-DAS-BIBLIOTECAS-IFES-2025.csv 
// python app.py
