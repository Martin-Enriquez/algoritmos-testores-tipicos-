#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace testores::domain {

// Subconjunto de columnas {0, ..., width-1} representado como bitset.
// Se usa tanto para representar filas de una matriz booleana (columnas con 1)
// como para representar conjuntos de rasgos (testores, conjuntos compatibles).
//
// El almacenamiento es un arreglo fijo en línea (sin memoria dinámica): las
// operaciones del ciclo interno de YYC crean muchos conjuntos temporales y
// evitar el heap las hace varias veces más rápidas.
class ColumnSet {
public:
    static constexpr std::size_t kMaxWidth = 512;

    ColumnSet() = default;
    // Lanza std::length_error si width > kMaxWidth.
    explicit ColumnSet(std::size_t width);

    static ColumnSet fromIndices(std::size_t width, const std::vector<std::size_t>& indices);

    std::size_t width() const noexcept { return width_; }

    // Lanzan std::out_of_range si col >= width().
    bool test(std::size_t col) const;
    void set(std::size_t col, bool value = true);

    std::size_t count() const noexcept;
    bool none() const noexcept;
    bool any() const noexcept { return !none(); }

    // Lanzan std::invalid_argument si los anchos difieren.
    bool intersects(const ColumnSet& other) const;
    bool isSubsetOf(const ColumnSet& other) const;
    ColumnSet& operator&=(const ColumnSet& other);
    ColumnSet& operator|=(const ColumnSet& other);

    // Índices (base 0) de las columnas presentes, en orden ascendente.
    std::vector<std::size_t> indices() const;

    friend bool operator==(const ColumnSet& a, const ColumnSet& b) noexcept;
    friend bool operator!=(const ColumnSet& a, const ColumnSet& b) noexcept { return !(a == b); }

private:
    static constexpr std::size_t kWordBits = 64;
    static constexpr std::size_t kMaxWords = kMaxWidth / kWordBits;

    void requireSameWidth(const ColumnSet& other) const;

    std::size_t width_ = 0;
    std::size_t wordCount_ = 0;                     // palabras en uso
    std::array<std::uint64_t, kMaxWords> words_{};  // las no usadas valen 0
};

ColumnSet operator&(ColumnSet a, const ColumnSet& b);
ColumnSet operator|(ColumnSet a, const ColumnSet& b);

// Orden canónico para presentar resultados: primero por cardinalidad y luego
// lexicográficamente por los índices de columna.
bool canonicalLess(const ColumnSet& a, const ColumnSet& b);

}  // namespace testores::domain
