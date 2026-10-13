#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <functional> // Para std::ref

// --- 1. UM CONTÊINER GENÉRICO THREAD-SAFE ---
// A mesma técnica (mutex + lock_guard) funciona para QUALQUER tipo T.
template <typename T>
class ListaSegura {
private:
    // O recurso compartilhado
    std::vector<T> itens_;

    // O porteiro que protege os itens.
    // 'mutable' permite travar o mutex também em métodos const.
    mutable std::mutex mtx_;

public:
    // Método 1: Inserir um item
    void inserir(const T& item) {
        // push_back pode realocar o vetor: sem o lock, duas threads
        // corromperiam a memória interna do std::vector
        std::lock_guard<std::mutex> lock(mtx_);
        itens_.push_back(item);
    }

    // Método 2: Inserir SOMENTE se ainda não existe (Check-then-Act)
    bool inserir_se_ausente(const T& item) {
        std::lock_guard<std::mutex> lock(mtx_);

        // CRITICAL SECTION: a busca e a inserção formam um bloco ATÔMICO
        for (const auto& x : itens_) {
            if (x == item) return false; // lock liberado aqui (RAII)
        }
        itens_.push_back(item);
        return true;                     // ...e aqui também
    }

    // Método 3: Tamanho (leitura protegida)
    std::size_t tamanho() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return itens_.size();
    }

    // Método 4: Cópia segura (snapshot) para uso fora do lock
    std::vector<T> copia() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return itens_; // a cópia é feita ANTES do lock ser liberado
    }
};

// --- 2. FUNÇÕES DE TESTE DE ESTRESSE ---

// Cada thread insere 'n' números. Todos devem aparecer no final.
void produtor_numeros(ListaSegura<int>& lista, int id, int n) {
    for (int i = 0; i < n; ++i) {
        lista.inserir(id * n + i);
    }
}

// Todas as threads tentam cadastrar os MESMOS nomes.
// Cada nome só pode entrar uma única vez.
void cadastrar_usuarios(ListaSegura<std::string>& usuarios, int n) {
    for (int i = 0; i < n; ++i) {
        usuarios.inserir_se_ausente("user_" + std::to_string(i));
    }
}

// --- 3. MAIN PARA EXECUTAR O DESAFIO ---
int main() {
    const int num_threads = 4;
    const int n = 2000;

    std::cout << "--- Desafio da Lista Genérica (lock_guard + template) ---\n";

    // Teste A: ListaSegura<int> com inserções simples
    ListaSegura<int> numeros;
    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; ++i) {
        workers.push_back(std::thread(produtor_numeros, std::ref(numeros), i, n));
    }
    for (auto& t : workers) t.join();
    workers.clear();

    std::cout << "\n[A] ListaSegura<int>\n";
    std::cout << "Esperado: " << num_threads * n << " | Obtido: " << numeros.tamanho() << "\n";

    // Teste B: ListaSegura<std::string> com Check-then-Act
    ListaSegura<std::string> usuarios;
    for (int i = 0; i < num_threads; ++i) {
        workers.push_back(std::thread(cadastrar_usuarios, std::ref(usuarios), n));
    }
    for (auto& t : workers) t.join();

    std::cout << "\n[B] ListaSegura<std::string> (sem duplicatas)\n";
    std::cout << "Esperado: " << n << " | Obtido: " << usuarios.tamanho() << "\n";

    bool ok = numeros.tamanho() == (std::size_t)num_threads * n &&
              usuarios.tamanho() == (std::size_t)n;

    if (ok) {
        std::cout << "\nStatus: SUCESSO! O lock_guard protegeu ambos os tipos.\n";
    } else {
        std::cout << "\nStatus: ERRO! Condição de corrida detectada.\n";
    }

    return 0;
}
