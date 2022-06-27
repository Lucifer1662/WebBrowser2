#pragma once
#include <memory>

struct Glyph;

struct poly_const_iterator
{
    using value_type = std::unique_ptr<Glyph>;

    struct concept {
        virtual void next(int n) = 0;
        virtual void prev(int n) = 0;
        virtual value_type& deref() = 0;
        virtual bool equal(const void* other) const = 0;
        virtual std::unique_ptr<concept> clone() const = 0;
        virtual const std::type_info& type() const = 0;
        virtual void* address() = 0;
        virtual ~concept() = default;
    };

    template<class Iter>
    struct model : concept
    {
        model(Iter iter) : _iter(iter) {}

        void next(int n) override { _iter = std::next(_iter, n); }
        void prev(int n) override { _iter = std::prev(_iter, n); }
        value_type& deref() override { return *_iter; }
        bool equal(const void* rp) const override { return _iter == static_cast<const model*>(rp)->_iter; }
        std::unique_ptr<concept> clone() const override { return std::make_unique<model>(*this); }
        const std::type_info& type() const override { return typeid(_iter); }
        void* address() override { return this; }


        Iter _iter;
    };

    std::unique_ptr<concept> _impl;

public:
    // interface

    // todo: constrain Iter to be something that iterates value_type
    template<class Iter>
    poly_const_iterator(Iter iter) : _impl(std::make_unique<model<Iter>>(iter)) {};

    poly_const_iterator(const poly_const_iterator& r) : _impl(r._impl->clone()) {};

    poly_const_iterator() {};

    value_type& operator*() {
        return _impl->deref();
    }

    value_type& operator->() {
        return _impl->deref();
    }

    poly_const_iterator& operator++() {
        _impl->next(1);
        return *this;
    }

    friend poly_const_iterator operator+(poly_const_iterator it, int num) {
        it._impl->next(num);
        return it;
    }
    friend poly_const_iterator operator-(poly_const_iterator it, int num) {
        it._impl->prev(num);
        return it;
    }


    poly_const_iterator& operator--() {
        _impl->prev(1);
        return *this;
    }

    poly_const_iterator& operator=(const poly_const_iterator& r) {
        _impl = std::move(r._impl->clone());
        return *this;
    }

    bool operator==(const poly_const_iterator& r) const {
        return _impl->type() == r._impl->type() && _impl->equal(r._impl->address());
    }

    bool operator != (const poly_const_iterator& r) const {
        return !(*this == r);
    }
};