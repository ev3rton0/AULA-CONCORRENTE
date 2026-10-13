#include <iostream>
#include <thread>
#include <mutex>
#include <deque> 
#include <chrono>
#include <functional> // Para std::ref
#include <condition_variable>

// Usa um tipo fixo (int) para simplificação.
class ControlePedidos {
private:
    std::mutex mtx_;
    std::deque<int> buffer_; 
    const size_t MAX_SIZE = 5;
    std::condition_variable permissaoConsumir;
    std::condition_variable permissaoProduzir;

public:

    // Método do Produtor (O garcom) - Tenta Inserir
    void put(const int& item) {
            // Tenta adquirir o Mutex para checar o estado (o Sino da Cozinha)
            std::unique_lock<std::mutex> lock(mtx_); // adquire o mutex

            // o wait_for() solta o mutex e espera a notificação, e quando acorda, re-adquire o mutex antes de continuar
            permissaoProduzir.wait(lock, [this] {return buffer_.size() < MAX_SIZE; }); 
  
            // --- SEÇÃO CRÍTICA PROTEGIDA ---
            buffer_.push_back(item); 
            printf("-> Garçom inseriu: %d. Tamanho do buffer é %lu  \n", item, buffer_.size());
            // --- FIM DA RG ---   
            permissaoConsumir.notify_one();
    }

    // Método do Consumidor (O chefe) - Tenta Retirar
    int get() {
            int item;
            std::unique_lock<std::mutex> lock(mtx_);
            permissaoConsumir.wait(lock, [this] {return !buffer_.empty();});
       
            // --- REGIÃO CRÍTICA PROTEGIDA ---
            item = buffer_.front();
            buffer_.pop_front(); 
            printf("<- Chefe removeu: %d. Tamanho do buffer %lu \n",item, buffer_.size());
            // --- FIM DA rC ---
            permissaoProduzir.notify_one();

            return item;
    }
};

// --- FUNÇÕES DE TESTE E MAIN ---

void tarefa_producao(ControlePedidos& buffer, int count) {
    for (int i = 1; i <= count; ++i) {
        buffer.put(i);
        std::this_thread::sleep_for(std::chrono::nanoseconds(1)); // Garçom Rápido
    }
}

void tarefa_consumo(ControlePedidos& buffer, int count) {
    for (int i = 0; i < count; ++i) {
        buffer.get();
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Chefe Lento (Força o Produtor a esperar)
    }
}

int main() {
    ControlePedidos buffer;
    const int num_items = 15;

    std::cout << "--- SIMULAÇÃO RESTAURANTE DO SEU ZÉ (MUTEX ONLY) ---\n";
    std::cout << "Capacidade do Buffer: 5 itens. Garçom é 10x mais rápido que o Chef.\n";

    std::thread produtor(tarefa_producao, std::ref(buffer), num_items);
    std::thread consumidor(tarefa_consumo, std::ref(buffer), num_items);

    produtor.join();
    consumidor.join();

    std::cout << "\n--- Simulação Concluída. --- \n";
    return 0;
}

