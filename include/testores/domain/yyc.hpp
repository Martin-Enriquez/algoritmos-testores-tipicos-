#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <vector>

#include "testores/domain/boolean_matrix.hpp"
#include "testores/domain/column_set.hpp"

namespace testores::domain {

// Algoritmo YYC (Alba-Cabrera, Ibarra-Fiallo, Godoy-Calderón, Cervantes-Alonso, 2014).
//
// Sea BM una matriz básica con filas r_1, ..., r_m. Se mantiene Ψ*, el conjunto
// de testores típicos de la submatriz formada por las filas ya procesadas:
//
//   Ψ* ← {{x_j} : r_1[j] = 1}
//   para k = 2..m:
//       Ψ_aux ← ∅
//       para cada τ ∈ Ψ*:
//           si τ ∩ r_k ≠ ∅:  Ψ_aux ← Ψ_aux ∪ {τ}
//           si no: para cada x con r_k[x] = 1:
//                  si τ ∪ {x} es compatible en las filas 1..k:  Ψ_aux ← Ψ_aux ∪ {τ ∪ {x}}
//       Ψ* ← Ψ_aux
//
// Al terminar, Ψ* es el conjunto de todos los testores típicos de BM.
// No se generan duplicados: si τ1 ∪ {x} = τ2 ∪ {y} con x ≠ y, entonces y ∈ τ1,
// lo cual contradice que τ1 no intersecta r_k.

class YycResourceLimitExceeded : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct YycOptions {
    // Registrar el detalle de cada paso (para explicaciones / ejecución manual).
    bool recordTrace = false;
    // Cota de seguridad sobre |Ψ*| para evitar agotar la memoria
    // (cada conjunto ocupa ~80 bytes: 2 millones ≈ 160 MB).
    std::size_t maxCompatibleSets = 2'000'000;
};

// Resultado de intentar extender un τ que no cubre la fila actual con x.
struct YycCandidate {
    std::size_t added = 0;  // x
    bool accepted = false;  // τ ∪ {x} es compatible
    // Si se rechazó: columna de τ ∪ {x} que quedó sin fila típica.
    std::optional<std::size_t> columnWithoutTypicalRow;
};

// Qué ocurrió con un τ de Ψ* al procesar la fila (en el orden de recorrido).
struct YycTauDecision {
    ColumnSet tau;
    bool coversRow = false;                // τ ∩ r_k ≠ ∅: se conserva tal cual
    std::vector<YycCandidate> extensions;  // vacío si coversRow
};

struct YycStepTrace {
    std::size_t rowIndex = 0;  // índice base 0 de la fila procesada
    // En la fila 1 no hay τ previos: Ψ* se inicializa con los {x_j} de r_1.
    std::vector<YycTauDecision> decisions;
};

struct YycStepStats {
    std::size_t rowIndex = 0;
    std::size_t kept = 0;
    std::size_t candidatesTried = 0;
    std::size_t candidatesAccepted = 0;
    std::size_t resultSize = 0;
};

class YycSolver {
public:
    // Lanza std::invalid_argument si la matriz está vacía.
    // No exige que la matriz sea básica (el algoritmo es correcto igual), pero
    // el enunciado del método asume matriz básica.
    explicit YycSolver(BooleanMatrix matrix, YycOptions options = {});

    bool finished() const noexcept { return processed_ == matrix_.rows(); }
    std::size_t processedRows() const noexcept { return processed_; }
    const BooleanMatrix& matrix() const noexcept { return matrix_; }

    // Procesa la siguiente fila. Lanza std::logic_error si ya terminó y
    // YycResourceLimitExceeded si |Ψ*| supera options.maxCompatibleSets.
    YycStepStats processNextRow();

    // Ψ* actual: testores típicos de las filas procesadas hasta ahora.
    const std::vector<ColumnSet>& current() const noexcept { return current_; }

    // Traza de la última fila procesada (vacía si recordTrace = false).
    const YycStepTrace& lastTrace() const noexcept { return lastTrace_; }

    // Ejecuta todo el algoritmo y devuelve los testores típicos en orden canónico.
    static std::vector<ColumnSet> solve(const BooleanMatrix& matrix, YycOptions options = {});

private:
    void initializeWithFirstRow(YycStepStats& stats);
    void enforceLimit(std::size_t size) const;

    BooleanMatrix matrix_;
    YycOptions options_;
    std::size_t processed_ = 0;
    std::vector<ColumnSet> current_;
    YycStepTrace lastTrace_;
};

}  // namespace testores::domain
