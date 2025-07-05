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
        cout << "Título: " << titulo << endl;
        cout << "Autor: " << (autor.empty() ? "Desconhecido" : autor) << endl;
        cout << "Editora: " << editora << endl;
        cout << "Ano: " << ano << endl;
        cout << "Exemplares: " << exemplares << endl;
        cout << "Classificação: " << classificacao << endl;
        cout << "Campus: " << campus << endl;
        cout << "Código: " << codigo << endl << endl;
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

// ------------------------ Main ----------------------------
int main(int argc, char* argv[]) {
    // Configurar locale para suportar caracteres acentuados
    setlocale(LC_ALL, "pt_BR.UTF-8");
    BPlusTree indiceTitulo;
    BPlusTree indiceAutor;
    unordered_map<int, Livro*> indiceCodigo;
    vector<Livro*> livros;

    // Verificar se o arquivo foi fornecido como parâmetro
    if (argc != 2) {
        cerr << "Uso: " << argv[0] << " <arquivo_csv>" << endl;
        cerr << "Exemplo: " << argv[0] << " ACERVO-DAS-BIBLIOTECAS-IFES-2025.csv" << endl;
        return 1;
    }

    // Ler arquivo CSV
    ifstream arquivo(argv[1]);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo: " << argv[1] << endl;
        return 1;
    }

    string linha;
    getline(arquivo, linha); // Pular cabeçalho

    while (getline(arquivo, linha)) {
        vector<string> campos = parseCSVLine(linha);
        
        if (campos.size() < 25) continue; // Linha inválida

        int codigo = 0;
        try {
            codigo = stoi(campos[0]);
        } catch (...) {
            continue; // Ignorar linhas com código inválido
        }

        string titulo = campos[2];
        string autor = extrairAutor(campos[1]);
        string editora = campos[24];
        int ano = extrairAno(campos[11]);
        
        int exemplares = 0;
        try {
            exemplares = stoi(campos[15]);
        } catch (...) {
            exemplares = 1;
        }
        
        string classificacao = campos[10];
        string campus = campos[20];

        Livro* livro = new Livro(codigo, titulo, autor, editora, ano, exemplares, classificacao, campus);
        livros.push_back(livro);
        indiceCodigo[codigo] = livro;

        // Indexar por palavras do título
        for (const auto& palavra : extrairPalavras(titulo)) {
            indiceTitulo.inserir(palavra, livro);
        }

        // Indexar por autor (se existir)
        if (!autor.empty()) {
            for (const auto& palavra : extrairPalavras(autor)) {
                indiceAutor.inserir(palavra, livro);
            }
        }
    }

    arquivo.close();

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
        cout << "| 4. Sair                                |" << endl;
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

        if (opcao == 4) break;

        switch (opcao) {
            case 1: {
                cout << "\n+----------------------------------------+" << endl;
                cout << "|      Buscar por palavra no título      |" << endl;
                cout << "+----------------------------------------+" << endl;
                cout << "Digite uma palavra para buscar no título: ";
                string palavra;
                getline(cin, palavra);
                transform(palavra.begin(), palavra.end(), palavra.begin(), ::tolower);
                
                vector<Livro*> resultado = indiceTitulo.buscarPrefixo(palavra);
                cout << "\n+----------------------------------------+" << endl;
                cout << "|      Resultados encontrados: " << resultado.size() << "         |" << endl;
                cout << "+----------------------------------------+" << endl;
                for (const auto* l : resultado) {
                    l->print();
                }
                break;
            }
            case 2: {
                cout << "\n+----------------------------------------+" << endl;
                cout << "|         Buscar por autor               |" << endl;
                cout << "+----------------------------------------+" << endl;
                cout << "Digite uma palavra para buscar no autor: ";
                string palavra;
                getline(cin, palavra);
                transform(palavra.begin(), palavra.end(), palavra.begin(), ::tolower);
                
                vector<Livro*> resultado = indiceAutor.buscarPrefixo(palavra);
                cout << "\n+----------------------------------------+" << endl;
                cout << "|      Resultados encontrados: " << resultado.size() << "         |" << endl;
                cout << "+----------------------------------------+" << endl;
                for (const auto* l : resultado) {
                    l->print();
                }
                break;
            }
            case 3: {
                cout << "\n+----------------------------------------+" << endl;
                cout << "|         Buscar por código              |" << endl;
                cout << "+----------------------------------------+" << endl;
                cout << "Digite o código do livro: ";
                int cod;
                cin >> cod;
                
                if (indiceCodigo.count(cod)) {
                    cout << "\n+----------------------------------------+" << endl;
                    cout << "|         Livro encontrado               |" << endl;
                    cout << "+----------------------------------------+" << endl;
                    indiceCodigo[cod]->print();
                } else {
                    cout << "\n+----------------------------------------+" << endl;
                    cout << "|      Livro não encontrado              |" << endl;
                    cout << "+----------------------------------------+" << endl;
                }
                break;
            }
            default:
                cout << "\n+----------------------------------------+" << endl;
                cout << "|         Opção inválida                 |" << endl;
                cout << "+----------------------------------------+" << endl;
                
        }
    }

    // Liberar memória
    for (auto* l : livros) delete l;

    return 0;
}
