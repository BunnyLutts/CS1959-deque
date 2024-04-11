#ifndef SJTU_DEQUE_HPP
#define SJTU_DEQUE_HPP

#include <cstddef>
#include <cmath>

#include "exceptions.hpp"

namespace sjtu {

    template <class T>
    class list {
	private:

		void dislink() {
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            prev = next = this;
		}

    public:

		static void erase(list<T> *p) {
			p->dislink();
			delete p;
		}

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

        list<T>* next_nth(int n) {
            list<T> *p = this;
            for (; n > 0; --n) p = p->next;
            return p;
        }

        const list<T>* next_nth(int n) const {
            const list<T> *p = this;
            for (; n > 0; --n) p = p->next;
            return p;
        }

        list<T>* prev_nth(int n) {
            list<T> *p = this;
            for (; n > 0; --n) p = p->prev;
            return p;
        }

        const list<T>* prev_nth(int n) const {
            const list<T> *p = this;
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
				dislink();
                delete tmp;
            }
            if (prev && prev!=this) {
                list *tmp = prev;
				dislink();
                delete tmp;
            }
        }
    };

    template <class T>
    class deque {
    private:
		struct block;
		struct node {
			T *data;
			list<block> *from;

			node() : data(nullptr), from(nullptr) {}
			node(T *data) : data(data), from(nullptr) {}
			node(const T &data) : data(new T(data)), from(nullptr) {}
			node(list<block> *from) : data(nullptr), from(from) {}

			~node() {
				if (data) delete data;
			}
		};

        struct block { 
            list<node> head;
            int size;

			block() : size(0) {}

            block *link_after(block *x) {
                x->head.next->prev = head.prev;
                x->head.prev->next = &head;
                head.prev->next = x->head.next;
                head.prev = x->head.prev;
                size += x->size;
                x->head.next = x->head.prev = &x->head;
                x->size = 0;
				delete x;
                return this;
            }

            block *cut_after(int pos) {
                list<node> *p = head.next_nth(pos);
                block *x = new block();
                x->head.next = p->next;
                x->head.prev = head.prev;
                x->head.next->prev = x->head.prev->next = &x->head;
                x->size = size - pos;
                p->next = &head;
                head.prev = p;
                size = pos;
				return x;
            }

			//Insert before
			list<node>* insert_before(list<node> *p, const T &data) {
				size++;
                list<node> *x = new list<node>(new node(data));
				p->insert_before(x);
                return x;
			}

			//Erase
			void erase(list<node> *p) {
				size--;
				list<node>::erase(p);
			}

            void pop_front() {
                size--;
				list<node>::erase(head->next);
            }

            void push_front(const T &data) {
                size++;
                head.insert_after(new list<T>(new node(data)));
            }

            void pop_back() {
                size--;
				list<node>::erase(head->prev);
            }

            void push_back(const T &data) {
                size++;
                head.insert_before(new list<node>(new node(data)));
            }
        };

        list<block> bs;
        int size_c, bsize;

		static list<block>* assignBlock(list<block> *x) {
            if (!x->data->head.data) {
                x->data->head.data = new node(x);
            } else {
                x->data->head.data->from = x;
            }
			return x;
		}

		static list<block>* makeBlock(block *x) {
			return assignBlock(new list<block>(x));
		}

		static list<block>* makeBlock() {
			return assignBlock(new list<block>(new block()));
		}

        void update(list<block> *x) {
            if (x == &bs) return;
            bsize = sqrt(size_c);

            //Split
            if (x->data->size > 2*bsize) {
                x->insert_after(makeBlock(x->data->cut_after(x->data->size/2)));
            }

            //Merge
            if (x->data->size < bsize) {
                if (x->prev != &bs && x->prev->data->size + x->data->size <= 2*bsize) {
                    auto *tmp = x->data, *p = x->prev;
                    x->data = nullptr;
					list<block>::erase(x);
                    p->data->link_after(tmp);
                } else if (x->next!= &bs && x->next->data->size + x->data->size <= 2*bsize) {
                    auto *tmp = x->next->data, *p = x;
                    x->next->data = nullptr;
					list<block>::erase(x->next);
                    p->data->link_after(tmp);
                }
            }
        }

        void copy(const deque &other) {
            size_c = other.size_c;
            bsize = sqrt(size_c);
            int cnt = bsize;
            for (auto it = other.cbegin(); it!=other.cend(); it++) {
                if (cnt==bsize) {
                    cnt = 0;
                    bs.insert_before(makeBlock());
                }
                bs.prev->data->push_back(*it);
                cnt++;
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
			list<node> *p;
            int cur;

            iterator(deque<T> *from, list<node> *p, int cur=0) : from(from), p(p), cur(cur) {}

			list<block>* findBlock() const {
				auto p1 = p;
				for (; !p1->data->from; p1 = p1->prev);
				return p1->data->from;
			}

        public:
            iterator() : from(nullptr), p(nullptr), cur(0) {}

            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            iterator operator+(const int &n) const {
				auto p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !p1->data->from; p1 = p1->next, nn--, ncur++);
				if (!p1->data->from) return iterator(from, p1, ncur);
				list<block> *pb = p1->data->from->next;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur += pb->data->size, pb = pb->next);
				return iterator(from, pb->data->head.next->next_nth(nn), ncur+nn);
				/*
                if (cur + n <= p1->data.size) {
                    return iterator(p1, p2->next_nth(n), cur+n);
                }
                n = n + cur - p1->data.size;
                p1 = p1->next;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->next;
                return iterator(p1, p1->data.head.next_nth(n), n);
				*/
            }
            iterator operator-(const int &n) const {
				auto p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !p1->data->from; p1 = p1->prev, nn--, ncur--);
				if (!p1->data->from) return iterator(from, p1, ncur);
				list<block> *pb = p1->data->from->prev;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur -= pb->data->size, pb = pb->prev);
				return iterator(from, pb->data->head.prev->prev_nth(nn), ncur-nn);
				/*
                if (cur-n > 0) {
                    return iterator(p1, p2->prev_nth(n), cur-n);
                }
                n = n-cur;
                p1 = p1->prev;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->prev;
                return iterator(p1, p1->data.head.prev_nth(n), n);
				*/
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const iterator &rhs) const {
                if (from != rhs.from) {
                    throw invalid_iterator();
                }
				return cur-rhs.cur;
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
                return *this = *this + 1;
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
                return *this = *this - 1;
            }

            /**
             * *it
             */
            T &operator*() const {
                return *p->data->data;
            }
            /**
             * it->field
             */
            T *operator->() const noexcept {
                return p->data->data;
            }

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {
				return p == rhs.p;
            }
            bool operator==(const const_iterator &rhs) const {
				return p == rhs.p;
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
            const deque<T> *from;
			const list<node> *p;
            int cur;

            const_iterator(const deque<T> *from, const list<node> *p, int cur=0) : from(from), p(p), cur(cur) {}

			list<block>* findBlock() const {
				auto p1 = p;
				for (; !p1->data->from; p1 = p1->prev);
				return p1->data->from;
			}
        public:
            const_iterator() : from(nullptr), p(nullptr), cur(0) {}
			const_iterator(const iterator &other) : from(other.from), p(other.p), cur(other.cur) {}

            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            const_iterator operator+(const int &n) const {
				auto p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !p1->data->from; p1 = p1->next, nn--, ncur++);
				if (!p1->data->from) return const_iterator(from, p1, ncur);
				list<block> *pb = p1->data->from->next;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur += pb->data->size, pb = pb->next);
				return const_iterator(from, pb->data->head.next->next_nth(nn), ncur+nn);
				/*
                if (cur + n <= p1->data.size) {
                    return iterator(p1, p2->next_nth(n), cur+n);
                }
                n = n + cur - p1->data.size;
                p1 = p1->next;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->next;
                return iterator(p1, p1->data.head.next_nth(n), n);
				*/
            }
            const_iterator operator-(const int &n) const {
				auto p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !p1->data->from; p1 = p1->prev, nn--, ncur--);
				if (!p1->data->from) return const_iterator(from, p1, ncur);
				list<block> *pb = p1->data->from->prev;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur -= pb->data->size, pb = pb->prev);
				return const_iterator(from, pb->data->head.prev->prev_nth(nn), ncur-nn);
				/*
                if (cur-n > 0) {
                    return iterator(p1, p2->prev_nth(n), cur-n);
                }
                n = n-cur;
                p1 = p1->prev;
                for (; n > p1->data.size; n-=p1->data.size) p1 = p1->prev;
                return iterator(p1, p1->data.head.prev_nth(n), n);
				*/
            }

            /**
             * return the distance between two iterators.
             * if they point to different vectors, throw
             * invaild_iterator.
             */
            int operator-(const const_iterator &rhs) const {
                if (from != rhs.from) {
                    throw invalid_iterator();
                }
				return cur-rhs.cur;
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
                return *this = *this + 1;
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
                return *this = *this - 1;
            }

            /**
             * *it
             */
            T &operator*() const {
                return *p->data->data;
            }
            /**
             * it->field
             */
            T *operator->() const noexcept {
                return p->data->data;
            }

            /**
             * check whether two iterators are the same (pointing to the same
             * memory).
             */
            bool operator==(const iterator &rhs) const {
				return p == rhs.p;
            }
            bool operator==(const const_iterator &rhs) const {
				return p == rhs.p;
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
        deque() : size_c(0), bsize(0), bs() {
            bs.data->head.insert_after(new list<node>());
			assignBlock(&bs);
            bs.data->size=1;
        }
        deque(const deque &other) {
            bs.data->head.insert_after(new list<node>());
			assignBlock(&bs);
            bs.data->size=1;
            copy(other);
        }

        /**
         * deconstructor.
         */
        ~deque() {}

        /**
         * assignment operator.
         */
        deque &operator=(const deque &other) {
            if (&other == this) return *this;
            clear();
            copy(other);
            return *this;
        }

        /**
         * access a specified element with bound checking.
         * throw index_out_of_bound if out of bound.
         */
        T &at(const size_t &pos) {
            if (pos >= size_c) {
                throw index_out_of_bound();
            }
            iterator it = begin() + pos;
            return *it;
        }
        const T &at(const size_t &pos) const {
            if (pos >= size_c) {
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
            return iterator(this, bs.next->data->head.next, 0);
        }
        const_iterator cbegin() const {
            return const_iterator(this, bs.next->data->head.next, 0);
        }

        /**
         * return an iterator to the end.
         */
        iterator end() {
            return iterator(this, bs.data->head.next, size_c);
        }
        const_iterator cend() const {
            return const_iterator(this, bs.data->head.next, size_c);
        }

        /**
         * check whether the container is empty.
         */
        bool empty() const {
            return size_c == 1;
        }

        /**
         * return the number of elements.
         */
        size_t size() const {
            return size_c-1;
        }

        /**
         * clear all contents.
         */
        void clear() {
            for (; bs.next != &bs; list<block>::erase(bs.next));
            size_c = 1;
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
			auto p1 = pos.findBlock();
            auto p2 = p1->data->insert_before(pos.p, value);
            size_c++;
            update(p1);
            return iterator(this, p2, pos.cur);
        }

        /**
         * remove the element at pos.
         * return an iterator pointing to the following element. if pos points to
         * the last element, return end(). throw if the container is empty,
         * the iterator is invalid, or it points to a wrong place.
         */
        iterator erase(iterator pos) {
            if (empty() || pos.from!=this) {
                throw invalid_iterator();
            }
            iterator nxt = pos+1;
			auto p1 = pos.findBlock();
            if (p1 == &bs) throw invalid_iterator();
            p1->data->erase(pos.p);
			nxt.cur--;
            size_c--;
            update(p1);
            return nxt;
        }

        /**
         * add an element to the end.
         */
        void push_back(const T &value) {
            insert(end(), value);
            size_c++;
            update(bs.prev);
        }

        /**
         * remove the last element.
         * throw when the container is empty.
         */
        void pop_back() {
            if (empty()) {
                throw container_is_empty();
            }
            erase(end()-1);
            size_c--;
            update(bs.prev);
        }

        /**
         * insert an element to the beginning.
         */
        void push_front(const T &value) {
            insert(begin(), value);
            size_c++;
            update(bs.next);
        }

        /**
         * remove the first element.
         * throw when the container is empty.
         */
        void pop_front() {
			if (empty()) {
                throw container_is_empty();
			}
            erase(begin());
            size_c--;
            update(bs.next);
        }
    };

}  // namespace sjtu

#endif
