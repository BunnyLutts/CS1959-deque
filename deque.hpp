#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include <cstddef>

#include "exceptions.hpp"

namespace sjtu {

    template <class T>
    class list {
    public:
        T *data;
        list *next, *prev;
        list() : data(nullptr) {
            next = prev = this;
        }
        list(T *data) : data(data) {
            next = prev = this;
        }
        list(const T &data) : data(new T(data)){
            next = prev = this;
        }
        list(T &&data) : data(new T(std::move(data))) {
            next = prev = this;
        }

        void insert_after(list *after) {
            after->next = this->next;
            after->prev = this;
            after->next->prev = after->prev->next = after;
        }

        void insert_before(list *before) {
            prev->insert_after(before);
        }

        void erase() {
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            prev = next = this;
        }

        ~list() {
            if (data) {
                delete data;
                data = nullptr;
            }
            if (next && next!=this) {
                list *tmp = next;
                erase();
                delete tmp;
            }
            if (prev && prev!=this) {
                list *tmp = prev;
                erase();
                delete tmp;
            }
        }
    };

    template <class T>
    class deque {
    private:
        struct block { 
            list<T> head;
            int size=0;

            block &link_after(block &x) {
                x.head->next->prev = head->prev;
                x.head->prev->next = head;
                head->prev->next = x.head->next;
                head->prev = x.head->prev;
                size += x.size;
                x.head->next = x.head->prev = &x.head;
                x.size = 0;
                return this;
            }

            block &link_before(block &x) {
                return x.link_after(*this);
            }

            block &cut_after(int pos) {
                list *p = &head;
                for (; pos > 0; p = p->next, --pos);
                block *x = new block;
                x.head->next = p->next;
                x.head->prev = head->prev;
                x.head->next->prev = x.head->prev->next = &x.head;
                x.size = size - pos;
                p->next = &head;
                head->prev = p;
                size = pos;
            }

            void pop_front() {
                size--;
                head->next->erase();
            }

            void push_front(const T &data) {
                size++;
                head.insert_after(new list<T>(data));
            }

            void pop_back() {
                size--;
                head->prev->erase();
            }

            void push_back(const T &data) {
                size++;
                head.insert_before(new list<T>(data));
            }
        };
        list<block> bs;
        int size, bsize;

    public:
        class const_iterator;
        class iterator {
            friend class deque;
        private:
            /**
             * add data members.
             * just add whatever you want.
             */
            list<block> *p1;
            list<T> *p2;

            iterator(list<block> *p1, list<T> *p2) : p1(p1), p2(p2) {}

        public:
            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            iterator operator+(const int &n) const {
            }
            iterator operator-(const int &n) const {
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const iterator &rhs) const {}
            iterator &operator+=(const int &n) {}
            iterator &operator-=(const int &n) {}

            /**
             * iter++
             */
            iterator operator++(int) {}
            /**
             * ++iter
             */
            iterator &operator++() {}
            /**
             * iter--
             */
            iterator operator--(int) {}
            /**
             * --iter
             */
            iterator &operator--() {}

            /**
             * *it
             */
            T &operator*() const {}
            /**
             * it->field
             */
            T *operator->() const noexcept {}

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {}
            bool operator==(const const_iterator &rhs) const {}
            /**
             * some other operator for iterators.
             */
            bool operator!=(const iterator &rhs) const {}
            bool operator!=(const const_iterator &rhs) const {}
        };

        class const_iterator {
            /**
             * it should has similar member method as iterator.
             * you can copy them, but with care!
             * and it should be able to be constructed from an iterator.
             */
        };

        /**
         * constructors.
         */
        deque() : size(0) {}
        deque(const deque &other) {}

        /**
         * deconstructor.
         */
        ~deque() {}

        /**
         * assignment operator.
         */
        deque &operator=(const deque &other) {}

        /**
         * access a specified element with bound checking.
         * throw index_out_of_bound if out of bound.
         */
        T &at(const size_t &pos) {}
        const T &at(const size_t &pos) const {}
        T &operator[](const size_t &pos) {}
        const T &operator[](const size_t &pos) const {}

        /**
         * access the first element.
         * throw container_is_empty when the container is empty.
         */
        const T &front() const {}
        /**
         * access the last element.
         * throw container_is_empty when the container is empty.
         */
        const T &back() const {}

        /**
         * return an iterator to the beginning.
         */
        iterator begin() {}
        const_iterator cbegin() const {}

        /**
         * return an iterator to the end.
         */
        iterator end() {}
        const_iterator cend() const {}

        /**
         * check whether the container is empty.
         */
        bool empty() const {}

        /**
         * return the number of elements.
         */
        size_t size() const {}

        /**
         * clear all contents.
         */
        void clear() {}

        /**
         * insert value before pos.
         * return an iterator pointing to the inserted value.
         * throw if the iterator is invalid or it points to a wrong place.
         */
        iterator insert(iterator pos, const T &value) {}

        /**
         * remove the element at pos.
         * return an iterator pointing to the following element. if pos points to
         * the last element, return end(). throw if the container is empty,
         * the iterator is invalid, or it points to a wrong place.
         */
        iterator erase(iterator pos) {}

        /**
         * add an element to the end.
         */
        void push_back(const T &value) {}

        /**
         * remove the last element.
         * throw when the container is empty.
         */
        void pop_back() {}

        /**
         * insert an element to the beginning.
         */
        void push_front(const T &value) {}

        /**
         * remove the first element.
         * throw when the container is empty.
         */
        void pop_front() {}
    };

}  // namespace sjtu

#endif
