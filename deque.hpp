#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include <cstddef>
#include <cmath>

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

        list<T>* next_nth(int n) const {
            list<T> *p = this;
            for (; n > 0; --n) p = p->next;
            return p;
        }

        list<T>* prev_nth(int n) const {
            list<T> *p = this;
            for (; n > 0; --n) p = p->prev;
            return p;
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

        void update(list<block> *x) {
            if (x == &bs) return;
            bsize = sqrt(size);

            //Split
            if (x->data.size > 2*bsize) {
                x->insert_after(x.data->cut(x->data.size/2));
            }

            //Merge
            if (x->data.size < bsize) {
                if (x->prev != &bs && x->prev->data.size + x->data.size <= 2*bsize) {
                    auto *tmp = x->data, *p = x->prev;
                    x->data = nullptr;
                    x->erase();
                    p->data.link_after(*tmp);
                } else if (x->next!= &bs && x->next->data.size + x->data.size <= 2*bsize) {
                    auto *tmp = x->next->data, *p = x;
                    x->next->data = nullptr;
                    x->next->erase();
                    p->data.link_before(*tmp);
                }
            }
        }

    public:
        class const_iterator;
        class iterator {
            friend class deque;
        private:
            /**
             * add data members.
             * just add whatever you want.
             */
            deque<T> *from;
            list<block> *p1;
            list<T> *p2;
            int cur;

            iterator(deque<T> *from, list<block> *p1, list<T> *p2, int cur=0) : from(from), p1(p1), p2(p2), cur(cur) {}

        public:
            iterator() : from(nullptr), p1(nullptr), p2(nullptr), cur(0) {}

            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            iterator operator+(const int &n) const {
                if (cur + n <= p1->data.size) {
                    return iterator(p1, p2->next_nth(n), cur+n);
                }
                n = n + cur - p1->data.size;
                p1 = p1->next;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->next;
                return iterator(p1, p1->data.head.next_nth(n), n);
            }
            iterator operator-(const int &n) const {
                if (cur-n > 0) {
                    return iterator(p1, p2->prev_nth(n), cur-n);
                }
                n = n-cur;
                p1 = p1->prev;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->prev;
                return iterator(p1, p1->data.head.prev_nth(n), n);
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const iterator &rhs) const {
                if (p1 == rhs.p1) {
                    return rhs.cur - cur;
                }
                if (from != rhs.from) {
                    throw invalid_iterator();
                }
                bool inv = false;
                list<block> *ptr = p1->next;
                int dis = (p1->data.size - cur) + rhs.cur;
                for (; ptr!=p2; dis+=ptr->data.size, ptr = ptr->next) {
                    if (ptr == &from->bs) {
                        inv = true;
                    }
                }
                return inv ? from->size - dis : dis;
            }
            iterator &operator+=(const int &n) {
                return *this = *this + n;
            }
            iterator &operator-=(const int &n) {
                return *this = *this - n;
            }

            /**
             * iter++
             */
            iterator operator++(int) {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            /**
             * ++iter
             */
            iterator &operator++() {
                if (p1->data.size == cur) {
                    p1 = p1->next;
                    p2 = &p1->data.head;
                    cur = 0;
                }
                p2 = p2->next;
                cur++;
                return *this;
            }
            /**
             * iter--
             */
            iterator operator--(int) {
                iterator tmp = *this;
                --(*this);
                return tmp;
            }
            /**
             * --iter
             */
            iterator &operator--() {
                if (cur == 1) {
                    p1 = p1->prev;
                    p2 = &p1->data.head;
                    cur = p1->data.size;
                }
                p2 = p2->prev;
                cur--;
                return *this;
            }

            /**
             * *it
             */
            T &operator*() const {
                return p2->data;
            }
            /**
             * it->field
             */
            T *operator->() const noexcept {
                return &p2->data;
            }

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {
                return p1 == rhs.p1 && p2 == rhs.p2;
            }
            bool operator==(const const_iterator &rhs) const {
                return p1 == rhs.p1 && p2 == rhs.p2;
            }
            /**
             * some other operator for iterators.
             */
            bool operator!=(const iterator &rhs) const {
                return !(*this == rhs);
            }
            bool operator!=(const const_iterator &rhs) const {
                return !(*this == rhs);
            }
        };

        class const_iterator {
            friend class deque;
            /**
             * it should has similar member method as iterator.
             * you can copy them, but with care!
             * and it should be able to be constructed from an iterator.
             */
            /*
             * Pasted...
             */
        private:
            /**
             * add data members.
             * just add whatever you want.
             */
            deque<T> *from;
            list<block> *p1;
            list<T> *p2;
            int cur;

            const_iterator(deque<T> *from, list<block> *p1, list<T> *p2, int cur=0) : from(from), p1(p1), p2(p2), cur(cur) {}

        public:
            const_iterator() : from(nullptr), p1(nullptr), p2(nullptr), cur(0) {}
            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            const_iterator operator+(const int &n) const {
                if (cur + n <= p1->data.size) {
                    return const_iterator(p1, p2->next_nth(n), cur+n);
                }
                n = n + cur - p1->data.size;
                p1 = p1->next;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->next;
                return const_iterator(p1, p1->data.head.next_nth(n), n);
            }
            const_iterator operator-(const int &n) const {
                if (cur-n > 0) {
                    return const_iterator(p1, p2->prev_nth(n), cur-n);
                }
                n = n-cur;
                p1 = p1->prev;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->prev;
                return const_iterator(p1, p1->data.head.prev_nth(n), n);
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const const_iterator &rhs) const {
                if (p1 == rhs.p1) {
                    return rhs.cur - cur;
                }
                if (from != rhs.from) {
                    throw invalid_iterator();
                }
                bool inv = false;
                list<block> *ptr = p1->next;
                int dis = (p1->data.size - cur) + rhs.cur;
                for (; ptr!=p2; dis+=ptr->data.size, ptr = ptr->next) {
                    if (ptr == &from->bs) {
                        inv = true;
                    }
                }
                return inv ? from->size - dis : dis;
            }
            const_iterator &operator+=(const int &n) {
                return *this = *this + n;
            }
            const_iterator &operator-=(const int &n) {
                return *this = *this - n;
            }

            /**
             * iter++
             */
            const_iterator operator++(int) {
                const_iterator tmp = *this;
                ++(*this);
                return tmp;
            }
            /**
             * ++iter
             */
            const_iterator &operator++() {
                if (p1->data.size == cur) {
                    p1 = p1->next;
                    p2 = &p1->data.head;
                    cur = 0;
                }
                p2 = p2->next;
                cur++;
                return *this;
            }
            /**
             * iter--
             */
            const_iterator operator--(int) {
                const_iterator tmp = *this;
                --(*this);
                return tmp;
            }
            /**
             * --iter
             */
            const_iterator &operator--() {
                if (cur == 1) {
                    p1 = p1->prev;
                    p2 = &p1->data.head;
                    cur = p1->data.size;
                }
                p2 = p2->prev;
                cur--;
                return *this;
            }

            /**
             * *it
             */
            const T &operator*() const {
                return p2->data;
            }
            /**
             * it->field
             */
            const T *operator->() const noexcept {
                return &p2->data;
            }

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {
                return p1 == rhs.p1 && p2 == rhs.p2;
            }
            bool operator==(const const_iterator &rhs) const {
                return p1 == rhs.p1 && p2 == rhs.p2;
            }
            /**
             * some other operator for iterators.
             */
            bool operator!=(const iterator &rhs) const {
                return !(*this == rhs);
            }
            bool operator!=(const const_iterator &rhs) const {
                return !(*this == rhs);
            }
        };

        /**
         * constructors.
         */
        deque() : size(0), bsize(0), bs() {
            bs.data.head.insert_after(new list<T>());
            bs.data.size=1;
        }
        deque(const deque &other) {
            bs.data.head.insert_after(new list<T>());
            bs.data.size=1;
            size = other.size,
            bsize = sqrt(size);
            int cnt = bsize;
            for (auto it = other.begin(); it!=other.end(); it++) {
                if (cnt==bsize) {
                    cnt = 0;
                    bs.insert_before(new block());
                }
                bs.prev->data->push_back(*it);
                cnt++;
            }
        }

        /**
         * deconstructor.
         */
        ~deque() {}

        /**
         * assignment operator.
         */
        deque &operator=(const deque &other) {
            if (&other == this) return;
            clear();
            size = other.size,
            bsize = sqrt(size);
            int cnt = bsize;
            for (auto it = other.begin(); it!=other.end(); it++) {
                if (cnt==bsize) {
                    cnt = 0;
                    bs.insert_before(new block());
                }
                bs.prev->data->push_back(*it);
                cnt++;
            }
        }

        /**
         * access a specified element with bound checking.
         * throw index_out_of_bound if out of bound.
         */
        T &at(const size_t &pos) {
            if (pos >= size) {
                throw index_out_of_bound();
            }
            iterator it = begin() + pos;
            return *it;
        }
        const T &at(const size_t &pos) const {
            if (pos >= size) {
                throw index_out_of_bound();
            }
            const_iterator it = cbegin() + pos;
            return *it;
        }
        T &operator[](const size_t &pos) {
            return at(pos);
        }
        const T &operator[](const size_t &pos) const {
            return at(pos);
        }

        /**
         * access the first element.
         * throw container_is_empty when the container is empty.
         */
        const T &front() const {
            return *cbegin();
        }
        /**
         * access the last element.
         * throw container_is_empty when the container is empty.
         */
        const T &back() const {
            return *(cend()-1);
        }

        /**
         * return an iterator to the beginning.
         */
        iterator begin() {
            return iterator(this, bs.next, bs.next->data.head.next);
        }
        const_iterator cbegin() const {
            return const_iterator(this, bs.next, bs.next->data.head.next);
        }

        /**
         * return an iterator to the end.
         */
        iterator end() {
            return iterator(this, &bs, &bs.data.head);
        }
        const_iterator cend() const {
            return const_iterator(this, &bs, &bs.data.head);
        }

        /**
         * check whether the container is empty.
         */
        bool empty() const {
            return size == 1;
        }

        /**
         * return the number of elements.
         */
        size_t size() const {
            return size-1;
        }

        /**
         * clear all contents.
         */
        void clear() {
            for (; bs.next != &bs; bs.next->erase());
            size = 1;
        }

        /**
         * insert value before pos.
         * return an iterator pointing to the inserted value.
         * throw if the iterator is invalid or it points to a wrong place.
         */
        iterator insert(iterator pos, const T &value) {
            if (pos.from!=this) {
                throw invalid_iterator();
            }
            pos.p1->data.insert_before(pos.p2, value);
            size++;
            update(pos.p1);
        }

        /**
         * remove the element at pos.
         * return an iterator pointing to the following element. if pos points to
         * the last element, return end(). throw if the container is empty,
         * the iterator is invalid, or it points to a wrong place.
         */
        iterator erase(iterator pos) {
            if (size == 1 || pos.from!=this || pos.p1 == &bs) {
                throw invalid_iterator();
            }
            iterator nxt = pos+1;
            pos.p1->data.erase(pos.p2);
            size--;
            update(pos.p1);
            return nxt;
        }

        /**
         * add an element to the end.
         */
        void push_back(const T &value) {
            insert(end(), value);
            size++;
            update(bs.prev);
        }

        /**
         * remove the last element.
         * throw when the container is empty.
         */
        void pop_back() {
            erase(end()-1);
            size--;
            update(bs.prev);
        }

        /**
         * insert an element to the beginning.
         */
        void push_front(const T &value) {
            insert(begin(), value);
            size++;
            update(bs.next);
        }

        /**
         * remove the first element.
         * throw when the container is empty.
         */
        void pop_front() {
            erase(begin());
            size--;
            update(bs.next);
        }
    };

}  // namespace sjtu

#endif
