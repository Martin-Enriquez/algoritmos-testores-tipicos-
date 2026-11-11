#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::domain {

// Algoritmo BT (Ruiz-Shulcloper, Bravo y Águila, 1982), algoritmo de escala exterior.
//
// Cada subconjunto de rasgos se codifica con un n-uplo booleano α = (a1, ..., an),
// con aj = 1 si xj ∈ α. Los n-uplos se recorren en el orden de los números
// naturales escritos en binario, con a1 como dígito MÁS significativo:
//     (0...01) = {xn},  (0...10) = {x(n-1)},  (0...11) = {x(n-1), xn},  ...,  (1...11).
// Para cada α se comprueba si es testor y, según el resultado, se "salta" en
// el conjunto potencia (apunte "Algo BT", Paso 3 y Proposiciones 1.3-1.5):
//
//   (S1) α NO es testor: hay filas con ceros en todas las columnas de α
//        (Prop. 1.3). Entre ellas se elige la que tiene el ÚLTIMO 1 MÁS A LA
//        IZQUIERDA (si hay empate, la de menor índice) y se toma k = posición
//        de ese último 1. Ningún n-uplo entre α y α' = (a1, ..., a(k-1), 1, 0, ..., 0)
//        es testor, porque todos siguen sin cubrir esa fila (Prop. 1.5).
//        Siguiente candidato: α'. Elegir el último 1 más a la izquierda da el
//        salto más grande posible.
//
//   (S2) α SÍ es testor. Sea k la posición del último 1 de α. Los n-uplos
//        α+1, ..., α+2^(n-k)-1 coinciden con α en las posiciones 1..k y sólo
//        agregan rasgos a la derecha de xk: son superconjuntos propios de α y,
//        por tanto, no son típicos (Prop. 1.4). Siguiente candidato: α + 2^(n-k)
//        ("sumar 1 en la posición del último 1", con acarreo).
//
// El algoritmo termina cuando el acarreo de (S2) se sale por la izquierda.
// Puede terminar antes de (1...11): si α = (1...10...0) ya es testor, los
// n-uplos restantes son superconjuntos suyos y no pueden ser típicos.
// La tipicidad de cada testor encontrado
// se decide en el momento con la caracterización "testor + cada rasgo tiene
// fila típica" (ver domain/testor.hpp).
//
// Si la matriz tiene una fila nula, ningún n-uplo la cubre: BT analiza (0...01),
// detecta la fila nula y termina sin testores.
//
// Como la fila de S1 se elige por la posición de su último 1 y no por su
// posición en la matriz, la secuencia de n-uplos analizados NO depende del
// orden de las filas (sólo podría cambiar cuál de dos filas empatadas se cita).
// No se reordenan filas ni columnas.

class BtResourceLimitExceeded : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct BtOptions {
    // Registrar cada n-uplo analizado (para la ejecución manual / explicación).
    bool recordTrace = false;
    // Máximo de pasos que se guardan en la traza; los siguientes sólo se cuentan.
    std::size_t maxTraceSteps = 100000;
    // Máximo de testores (típicos o no) que se guardan en la lista de testores
    // encontrados; superado, sólo se cuentan (los típicos se guardan siempre).
    std::size_t maxStoredTestors = 2'000'000;
    // Cota de seguridad sobre la cantidad de testores típicos guardados.
    std::size_t maxTypicalTestors = 2'000'000;
    // Cota de trabajo, independiente del tamaño de las listas guardadas.
    std::size_t maxCandidatesEvaluated = std::numeric_limits<std::size_t>::max();
    // Cancelación cooperativa: true interrumpe con BtResourceLimitExceeded.
    // Se consulta al iniciar y cada 1024 candidatos; el dominio no conoce
    // el reloj ni la política que usa el llamador para decidir el límite.
    std::function<bool()> shouldStop;
};

enum class BtVerdict {
    NotTestor,         // existe una fila de ceros en la submatriz
    TypicalTestor,     // testor y todos sus rasgos tienen fila típica
    NonTypicalTestor   // testor, pero algún rasgo no tiene fila típica
};

struct BtStep {
    ColumnSet candidate;  // α
    BtVerdict verdict = BtVerdict::NotTestor;
    // NotTestor: fila (base 0) que provoca el salto S1: entre las filas con
    // ceros en todas las columnas de α, la de último 1 más a la izquierda.
    std::size_t zeroRow = 0;
    // NotTestor: zeroRow es una fila nula; ningún n-uplo la cubre y BT termina.
    bool zeroRowIsNull = false;
    // Columna (base 0) cuya posición determina el salto: el último 1 de la
    // fila de ceros (S1) o el último 1 de α (S2). Sin significado si zeroRowIsNull.
    std::size_t pivot = 0;
    // NonTypicalTestor: un rasgo de α sin fila típica.
    std::optional<std::size_t> columnWithoutTypicalRow;
    // Siguiente candidato; vacío si el algoritmo termina.
    std::optional<ColumnSet> next;
};

struct BtFoundTestor {
    ColumnSet set;
    bool typical = false;
};

struct BtStats {
    std::size_t candidatesEvaluated = 0;  // n-uplos analizados
    std::size_t testorsFound = 0;         // de ellos, cuántos son testores
    std::size_t typicalFound = 0;         // y cuántos testores típicos
};

struct BtResult {
    std::vector<BtFoundTestor> testors;    // testores analizados, en el orden de BT
    bool testorsTruncated = false;         // true si se superó maxStoredTestors
    std::vector<ColumnSet> typicalTestors; // orden canónico
    std::vector<BtStep> trace;             // vacío si recordTrace = false
    bool traceTruncated = false;
    BtStats stats;
};

// Valor numérico del n-uplo α (a1 = bit más significativo). Vacío si n > 63.
std::optional<std::uint64_t> btNumber(const ColumnSet& alpha);

// Fila que provoca el salto S1 para α: entre las filas con ceros en todas las
// columnas de α, la de último 1 más a la izquierda (una fila nula cuenta como
// la más a la izquierda); en empate, la de menor índice. Vacío si α es testor.
std::optional<std::size_t> btZeroRow(const BooleanMatrix& matrix, const ColumnSet& alpha);

// Salto (S1): 1 en `pivot` y ceros a su derecha. Requiere que alpha no contenga pivot.
ColumnSet btJumpNotTestor(const ColumnSet& alpha, std::size_t pivot);

// Salto (S2): α + 2^(n-k), con k = último 1 de α. Vacío si hay desbordamiento
// (α era el último n-uplo que comienza con esos unos: fin del recorrido).
std::optional<ColumnSet> btJumpTestor(const ColumnSet& alpha);

class BtSolver {
public:
    // Lanza std::invalid_argument si la matriz está vacía. Con una fila nula
    // devuelve cero testores (ningún subconjunto de rasgos la cubre).
    static BtResult solve(const BooleanMatrix& matrix, const BtOptions& options = {});
};

// Elementos minimales (respecto de ⊆) de una familia de conjuntos, en orden
// canónico. Aplicado a los testores que encuentra BT da exactamente los
// testores típicos: es el procedimiento "a posteriori" de identificación.
std::vector<ColumnSet> minimalSets(const std::vector<ColumnSet>& sets);

}  // namespace testores::domain
