#pragma once

template <typename T = void> struct VoidType { using type = T; };

template <> struct VoidType<void> {
    using type = VoidType;

    explicit VoidType() = default;
};
