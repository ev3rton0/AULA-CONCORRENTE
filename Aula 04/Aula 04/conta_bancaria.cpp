#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional> // Para std::ref

// --- 1. A CLASSE CONTA BANCÁRIA ---
class ContaBancaria {
private:
    // O recurso compartilhado
    long long saldo_ = 10000; 

    // O porteiro que protege o saldo. Deve ser membro privado.
    std::mutex mtx_;        

public:
    // Método 1: Depositar
    void depositar(long long valor) {
        // Usa RAII: O lock é liberado automaticamente no fim do escopo
        std::lock_guard<std::mutex> lock(mtx_); 
        saldo_ += valor;
    }

    // Método 2: Sacar (A Seção Crítica de "Checagem e Ação")
    bool sacar(long long valor) {
        // --- ADQUIRE O LOCK (RAII) ---
        // Garante que o if e o saque sejam um bloco ATÔMICO
        std::lock_guard<std::mutex> lock(mtx_); 

        // CRITICAL SECTION (Check-then-Act)
        if (saldo_ >= valor) {
            saldo_ -= valor;
            return true;
        }
        return false; // Falhou na checagem
    }

    // Método 3: Checar Saldo
    long long get_saldo() {
        // Proteção para garantir que a leitura não seja interrompida
        std::lock_guard<std::mutex> lock(mtx_); 
        return saldo_;
    }
};

// --- 2. FUNÇÃO DE TESTE DE ESTRESSE ---
void stress_transactions(ContaBancaria& conta, int num_transacoes) {
    for (int i = 0; i < num_transacoes; ++i) {
        // Tentativa de depositar 100
        conta.depositar(100);
        
        // Tentativa de sacar 50
        conta.sacar(50);
        
        // Aumentar e diminuir para o saldo esperado ser previsível
    }
}

// --- 3. MAIN PARA EXECUTAR O DESAFIO ---
int main() {
    ContaBancaria conta;
    const int num_transacoes_per_thread = 50000;
    const int num_threads = 4;
    
    std::cout << "--- Desafio da Conta Bancária (Mutex RAII) ---\n";
    std::cout << "Saldo Inicial: R$" << conta.get_saldo() << "\n";

    std::vector<std::thread> workers;
    
    // Inicia as threads (passa a conta por referência - std::ref)
    for (int i = 0; i < num_threads; ++i) {
        workers.push_back(std::thread(stress_transactions, 
                                      std::ref(conta), 
                                      num_transacoes_per_thread));
    }

    // Espera todas as threads terminarem
    for (auto& t : workers) {
        t.join();
    }
    
    // Cálculo do Saldo Esperado:
    // Saldo Inicial + (Total de Threads * Transações por Thread * (100 - 50))
    long long total_ganho_por_operacao = 100 - 50;
    long long total_operado = (long long)num_threads * num_transacoes_per_thread * total_ganho_por_operacao;
    long long expected_saldo = 10000 + total_operado;

    std::cout << "\n--- Resultados ---\n";
    std::cout << "Saldo Esperado (Correto): R$" << expected_saldo << "\n";
    std::cout << "Saldo Final Obtido:       R$" << conta.get_saldo() << "\n";
    
    if (conta.get_saldo() == expected_saldo) {
        std::cout << "\nStatus: SUCESSO! A Exclusão Mútua garantiu a consistência bancária.\n";
    } else {
        std::cout << "\nStatus: ERRO! Falha na consistência.\n";
    }

    return 0;
}