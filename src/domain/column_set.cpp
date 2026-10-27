#include "testores/domain/column_set.hpp"

#include <bitset>
#include <stdexcept>

namespace testores::domain {

ColumnSet::ColumnSet(std::size_t width) : width_(width), wordCount_((width + kWordBits - 1) / kWordBits) {
    if (width > kMaxWidth) {
        throw std::length_error("ColumnSet: se admiten como máximo 512 columnas");
    }
}

ColumnSet ColumnSet::fromIndices(std::size_t width, const std::vector<std::size_t>& indices) {
    ColumnSet result(width);
    for (std::size_t col : indices) {
        result.set(col);
    }
    return result;
}

bool ColumnSet::test(std::size_t col) const {
    if (col >= width_) {
        throw std::out_of_range("ColumnSet::test: columna fuera de rango");
    }
    return ((words_[col / kWordBits] >> (col % kWordBits)) & 1U) != 0;
}

void ColumnSet::set(std::size_t col, bool value) {
    if (col >= width_) {
        throw std::out_of_range("ColumnSet::set: columna fuera de rango");
    }
    const std::uint64_t mask = std::uint64_t{1} << (col % kWordBits);
    if (value) {
        words_[col / kWordBits] |= mask;
    } else {
        words_[col / kWordBits] &= ~mask;
    }
}

std::size_t ColumnSet::count() const noexcept {
    std::size_t total = 0;
    for (std::size_t i = 0; i < wordCount_; ++i) {
        total += std::bitset<kWordBits>(words_[i]).count();
    }
    return total;
}

bool ColumnSet::none() const noexcept {
    for (std::size_t i = 0; i < wordCount_; ++i) {
        if (words_[i] != 0) {
            return false;
        }
    }
    return true;
}

void ColumnSet::requireSameWidth(const ColumnSet& other) const {
    if (width_ != other.width_) {
        throw std::invalid_argument("ColumnSet: operación entre conjuntos de distinto ancho");
    }
}

bool ColumnSet::intersects(const ColumnSet& other) const {
    requireSameWidth(other);
    for (std::size_t i = 0; i < wordCount_; ++i) {
        if ((words_[i] & other.words_[i]) != 0) {
            return true;
        }
    }
    return false;
}

bool ColumnSet::isSubsetOf(const ColumnSet& other) const {
    requireSameWidth(other);
    for (std::size_t i = 0; i < wordCount_; ++i) {
        if ((words_[i] & ~other.words_[i]) != 0) {
            return false;
        }
    }
    return true;
}

ColumnSet& ColumnSet::operator&=(const ColumnSet& other) {
    requireSameWidth(other);
    for (std::size_t i = 0; i < wordCount_; ++i) {
        words_[i] &= other.words_[i];
    }
    return *this;
}

ColumnSet& ColumnSet::operator|=(const ColumnSet& other) {
    requireSameWidth(other);
    for (std::size_t i = 0; i < wordCount_; ++i) {
        words_[i] |= other.words_[i];
    }
    return *this;
}

std::vector<std::size_t> ColumnSet::indices() const {
    std::vector<std::size_t> result;
    for (std::size_t col = 0; col < width_; ++col) {
        if (test(col)) {
            result.push_back(col);
        }
    }
    return result;
}

bool operator==(const ColumnSet& a, const ColumnSet& b) noexcept {
    // Las palabras no usadas siempre valen 0, así que comparar el arreglo completo es correcto.
    return a.width_ == b.width_ && a.words_ == b.words_;
}

ColumnSet operator&(ColumnSet a, const ColumnSet& b) { return a &= b; }

ColumnSet operator|(ColumnSet a, const ColumnSet& b) { return a |= b; }

bool canonicalLess(const ColumnSet& a, const ColumnSet& b) {
    const std::size_t ca = a.count();
    const std::size_t cb = b.count();
    if (ca != cb) {
        return ca < cb;
    }
    return a.indices() < b.indices();
}

}  // namespace testores::domain
