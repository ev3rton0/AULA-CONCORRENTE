#include <iostream>
#include <thread>
#include <mutex> // Essencial para a solução correta
#include <vector>

// --- Nossas variáveis compartilhadas ---

// O Mutex (Porteiro Bloqueante)
std::mutex g_mtx; 

// O recurso compartilhado (contador)
long long g_contador = 0;

// Constante de iterações, mantendo o valor do arquivo anterior
const int ITERACOES = 100000; 

// Número de threads
const int NUM_THREADS = 100;

/**
 * @brief Tenta incrementar o contador usando a solução correta (Bloqueante)
 */
void incrementar() {
    for (int i = 0; i < ITERACOES; ++i) {
        
        // --- Protocolo de Entrada: BLOQUEIA (Se estiver ocupado, a thread dorme) ---
        g_mtx.lock(); 
        
        // --- Início da Seção Crítica ---
        // A Exclusão Mútua está garantida por este ponto
        g_contador++;
        // --- Fim da Seção Crítica ---

        // --- Protocolo de Saída: LIBERA (Acorda a thread que está esperando) ---
        g_mtx.unlock(); 
    }
}

// Versão 2: RAII (Segura e Recomendada)
void incrementar_raii() {
    for (int i = 0; i < 100000; ++i) {
        // Cria um escopo limitado para a seção crítica
        {
            // O construtor chama g_mtx.lock()
            std::lock_guard<std::mutex> guarda(g_mtx);
            
            g_contador++;
            
            // Ao chegar aqui (fim do escopo), o destrutor de 'guarda'
            // é chamado e executa g_mtx.unlock() automaticamente.
        }
    }
}


/**
 * @brief Função 'main' para testar a solução do Mutex.
 */
int main() {
    std::cout << "Iniciando "<< NUM_THREADS << " threads, cada uma com " << ITERACOES << " iterações...\n";

    // Cria e inicia as threads
    std::vector<std::thread> minhasThreads;

    for(int i=0; i< NUM_THREADS; i++){
        minhasThreads.emplace_back(incrementar_raii); // insere um novo elemento std::thread ao final,
                                                 // construindo-o diretamente no local. 
    }

    // execução sequencial
    // for(int i=0; i< NUM_THREADS; i++){
    //     incrementar();
    // }

    // Espera que ambas terminem
    for(std::thread &thisThread : minhasThreads){ // itera em cada elemento do conteiner
        thisThread.join();
    }

    // Imprime o resultado
    long long valor_esperado = (long long)ITERACOES * NUM_THREADS;
    
    std::cout << "Valor esperado:   " << valor_esperado << std::endl;
    std::cout << "Valor obtido:     " << g_contador << std::endl;

    if (g_contador == valor_esperado) {
        std::cout << "\nSucesso! A Exclusão Mútua funcionou.\n";
    } else {
        std::cout << "\nFalha! A exclusão mútua falhou (o que não deveria acontecer com std::mutex).\n";
    }

    return 0;
}