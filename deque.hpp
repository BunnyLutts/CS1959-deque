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
        static const int BSIZE = 128;

		struct block;
		struct node : list<T> {
			list<block> *from;

			node() : list<T>(), from(nullptr) {}
			node(T *data) : list<T>(data), from(nullptr) {}
			node(const T &data) : list<T>(data), from(nullptr) {}
			node(list<block> *from) : list<T>(), from(from) {}
		};

        static node* cas(list<T> *p) {
            return reinterpret_cast<node *>(p);
        }

        static const node* cas(const list<T> *p) {
            return reinterpret_cast<const node *>(p);
        }
        
        static list<block>* nfrom(list<T> *p) {
            return reinterpret_cast<node *>(p)->from;
        }

        static const list<block>* nfrom(const list<T> *p) {
            return reinterpret_cast<const node *>(p)->from;
        }

        struct block { 
            node head;
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
                list<T> *p = head.next_nth(pos);
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
			list<T>* insert_before(list<T> *p, const T &data) {
				size++;
                node *x = new node(data);
				p->insert_before(x);
                return x;
			}

			//Erase
			void erase(list<T> *p) {
				size--;
				node::erase(cas(p));
			}

            void pop_front() {
                size--;
				node::erase(cas(head->next));
            }

            void push_front(const T &data) {
                size++;
                head.insert_after(new node(data));
            }

            void pop_back() {
                size--;
				list<node>::erase(cas(head->prev));
            }

            void push_back(const T &data) {
                size++;
                head.insert_before(new node(data));
            }
        };

        list<block> bs;
        int size_c, bsize;

		static list<block>* assignBlock(list<block> *x) {
            x->data->head.from = x;
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
            bsize = BSIZE*BSIZE < size_c ? sqrt(size_c) : BSIZE;

            if (x->data->size == 0) {
                list<block>::erase(x);
                return;
            }

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
			list<T> *p;
            int cur;

            iterator(deque<T> *from, list<T> *p, int cur=0) : from(from), p(p), cur(cur) {}

			list<block>* findBlock() const {
				list<T> *p1 = p;
				for (; !nfrom(p1); p1 = p1->prev);
				return nfrom(p1);
			}

        public:
            iterator() : from(nullptr), p(nullptr), cur(0) {}

            /**
             * return a new iterator which points to the n-next element.
             * if there are not enough elements, the behaviour is undefined.
             * same for operator-.
             */
            iterator operator+(const int &n) const {
                if (n<0) return *this - (-n);
                if (cur+n >= from->size_c) {
                    throw index_out_of_bound();
                }
				list<T> *p1 = p;
				int nn = n;

                //Special for begin() + n
                if (nfrom(p1->prev) && nn >= nfrom(p1->prev)->data->size) {
                    nn-=nfrom(p1->prev)->data->size;
                    p1 = p1->prev;
                } else {
                    for (; nn && !nfrom(p1); p1 = p1->next, nn--)
                        ;
                    if (!nfrom(p1)) return iterator(from, p1, cur+n);
                }
                list<block> *pb = nfrom(p1)->next;
				for (; nn>=pb->data->size; nn -= pb->data->size, pb = pb->next);
				return iterator(from, pb->data->head.next->next_nth(nn), cur+n);
            }
            iterator operator-(const int &n) const {
                if (n<0) return *this + (-n);
                if (cur-n < 0) {
                    throw index_out_of_bound();
                }
				list<T> *p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !nfrom(p1); p1 = p1->prev, nn--, ncur--);
				if (!nfrom(p1)) return iterator(from, p1, ncur);
				list<block> *pb = nfrom(p1)->prev;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur -= pb->data->size, pb = pb->prev);
				return iterator(from, pb->data->head.prev->prev_nth(nn), ncur-nn);
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
                if (!p->data) {
                    throw invalid_iterator();
                }
                return *p->data;
            }
            /**
             * it->field
             */
            T *operator->() const noexcept {
                if (!p->data) {
                    throw invalid_iterator();
                }
                return p->data;
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
			const list<T> *p;
            int cur;

            const_iterator(const deque<T> *from, const list<T> *p, int cur=0) : from(from), p(p), cur(cur) {}

			list<block>* findBlock() const {
				auto p1 = p;
				for (; !nfrom(p1); p1 = p1->prev);
				return nfrom(p1);
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
                if (n<0) return *this - (-n);
                if (cur+n >= from->size_c) {
                    throw index_out_of_bound();
                }
				auto *p1 = p;
				int nn = n;

                //Special for begin() + n
                if (nfrom(p1->prev) && nn >= nfrom(p1->prev)->data->size) {
                    nn-=nfrom(p1->prev)->data->size;
                    p1 = p1->prev;
                } else {
                    for (; nn && !nfrom(p1); p1 = p1->next, nn--)
                        ;
                    if (!nfrom(p1)) return const_iterator(from, p1, cur+n);
                }
                list<block> *pb = nfrom(p1)->next;
				for (; nn>=pb->data->size; nn -= pb->data->size, pb = pb->next);
				return const_iterator(from, pb->data->head.next->next_nth(nn), cur+n);
            }
            const_iterator operator-(const int &n) const {
                if (n<0) return *this + (-n);
                if (cur-n < 0) {
                    throw index_out_of_bound();
                }
				auto *p1 = p;
				int ncur = cur, nn = n;
				for (; nn && !nfrom(p1); p1 = p1->prev, nn--, ncur--);
				if (!nfrom(p1)) return const_iterator(from, p1, ncur);
				list<block> *pb = nfrom(p1)->prev;
				for (; nn>=pb->data->size; nn -= pb->data->size, ncur -= pb->data->size, pb = pb->prev);
				return const_iterator(from, pb->data->head.prev->prev_nth(nn), ncur-nn);
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
                if (!p->data) {
                    throw invalid_iterator();
                }
                return *p->data;
            }
            /**
             * it->field
             */
            T *operator->() const noexcept {
                if (!p->data) {
                    throw invalid_iterator();
                }
                return p->data;
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
        deque() : size_c(1) {
            bs.data = new block();
            bs.data->head.insert_after(new node());
			assignBlock(&bs);
            bs.data->size=1;
        }
        deque(const deque &other) : size_c(1) {
            bs.data = new block();
            bs.data->head.insert_after(new node());
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
            if (pos >= size_c-1) {
                throw index_out_of_bound();
            }
            iterator it = begin() + pos;
            return *it;
        }
        const T &at(const size_t &pos) const {
            if (pos >= size_c-1) {
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
            return iterator(this, bs.data->head.next, size_c-1);
        }
        const_iterator cend() const {
            return const_iterator(this, bs.data->head.next, size_c-1);
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
			auto p1 = pos.findBlock(), p = pos.p;
            if (p1 == &bs) {
                if (p1->prev == &bs) {
                    p1->insert_before(makeBlock());
                }
                p1 = p1->prev;
                p = &p1->data->head;
            }
            auto p2 = p1->data->insert_before(p, value);
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
            // insert(end(), value);
            if (bs.prev == &bs) {
                bs.insert_before(makeBlock());
            }
            bs.prev->data->insert_before(&bs.prev->data->head, value);
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
            // erase(end()-1);
            bs.prev->data->erase(bs.prev->data->head.prev);
            size_c--;
            update(bs.prev);
        }

        /**
         * insert an element to the beginning.
         */
        void push_front(const T &value) {
            // insert(begin(), value);
            if (bs.next == &bs) {
                bs.insert_after(makeBlock());
            }
            bs.next->data->insert_before(bs.next->data->head.next, value);
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
            // erase(begin());
            bs.next->data->erase(bs.next->data->head.next);
            size_c--;
            update(bs.next);
        }
    };

}  // namespace sjtu

#endif
