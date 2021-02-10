// Copyright
#ifndef SOURCE_B_LINK_HPP_
#define SOURCE_B_LINK_HPP_
#include <iostream>
#include <queue>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <condition_variable>
#include <deque>
#include <thread>
#include <queue>
#include <random>

#include <utility>
#define d 2
using namespace std;

class TreeNode {
public:
	shared_mutex mutex;
	int cant_llave, cant_ptr;
	vector<int>key;
	TreeNode* izq;
	TreeNode* der;
	vector<TreeNode*>ptr;
	bool hoja;

	TreeNode(bool hoja) {
		cant_llave = 0;
		cant_ptr = 0;
		key.resize(2 * d + 1);
		ptr.resize(2 * d + 2, NULL);
		this->hoja = hoja;
		izq = der = NULL;
	}

	int obtener_ultima_k() { return key[cant_llave - 1]; }
	int obtener_primera_k() { return key[0]; }

	void obtener(TreeNode* t2) {
		t2->izq = this;
		t2->der = this->der;
		this->der = t2;
		if (t2->der)
			t2->der->izq = t2;
	}

	void insertar_k(const int& value) {
		int i;
		for (i = cant_llave; i >= 1; i--) {
			if (cant_llave != key.size()) {
				if (key[i - 1] > value) {
					key[i] = key[i - 1];
				}
				else {
					key[i] = value;
					break;
				}
			}
		}
		if (!i)
			key[i] = value;
		cant_llave++;
	}

	void insertar_p(TreeNode* t) {
		int i;
		for (i = cant_ptr; i >= 1; i--) {
			if (ptr[i - 1]->key[0] > t->key[0]) {
				ptr[i] = ptr[i - 1];
			}
			else {
				ptr[i] = t;
				break;
			}
		}
		if (!i)
			ptr[i] = t;
		cant_ptr++;
	}

	int eliminar_k() {
		cant_llave--;
		return key[cant_llave];
	}

	int eliminar_k1() {
		int frontKey = key[0];
		for (int i = 1; i < cant_llave; i++)
			key[i - 1] = key[i];
		cant_llave--;
		return frontKey;
	}
	TreeNode* eliminar_p() {
		cant_ptr--;
		return ptr[cant_ptr];
	}

	bool eliminar_k2(const int& t) {
		bool encontrado = 0;
		for (int i = 0; i < cant_llave; i++) {
			if (encontrado) {
				key[i - 1] = key[i];
				continue;
			}
			if (key[i] == t) {
				encontrado = 1;
			}
		}
		cant_llave -= encontrado;
		return encontrado;
	}
};


//EL blink paralelo

namespace EDA {
	namespace Concurrent {
		template <std::size_t B, typename Type>
		class BLinkTree {
		public:
			typedef Type data_type;
			TreeNode* raiz;
			BLinkTree() {raiz = new TreeNode(true);}

			bool search(const data_type& value) const {
				return buscar(value, raiz, NULL);
			}

			void insert(const data_type& value) {
				queue<TreeNode*> queue_;
				TreeNode* child = insertar(value, raiz, queue_);
				if (raiz->hoja && child && child != raiz) {
					TreeNode* nueva_raiz = new TreeNode(false);
					nueva_raiz->mutex.lock();
					nueva_raiz->insertar_k(child->value[0]);
					nueva_raiz->insertar_p(raiz);
					nueva_raiz->insertar_p(child);
					raiz = nueva_raiz;
					nueva_raiz->mutex.unlock();
				}
				desbloq2(queue_);
			}

			void remove(const data_type& value) {}

		private:
			data_type* data_;
			bool buscar(const int& key, TreeNode* ptr, TreeNode* parent) {
				if (!parent) {
					ptr->mutex.lock_shared();
				}
				else {
					ptr->mutex.lock_shared();
					parent->mutex.unlock_shared();
				}
				if (ptr->hoja) {
					for (int i = 0; i < ptr->cant_llave; i++)
						if (ptr->key[i] == key) {
							ptr->mutex.unlock_shared();
							return true;
						}
					ptr->mutex.unlock_shared();
					return false;
				}
				if (!ptr->cant_llave) {
					ptr->mutex.unlock_shared();
					return false;
				}
				if (key < ptr->key[0])
					return buscar(key, ptr->ptr[0], ptr);
				if (key > ptr->key[ptr->cant_llave])
					return buscar(key, ptr->ptr[ptr->cant_llave], ptr);
				for (int i = 0; i < ptr->cant_llave - 1; i++)
					if (key >= ptr->key[i] && key < ptr->key[i + 1])
						return buscar(key, ptr->ptr[i + 1], ptr);
				ptr->mutex.unlock_shared();
				return false;
			}

			void desbloq1(queue<TreeNode*>& queue_) {
				for (int i = 0; i < queue_.size(); i++) {
					queue_.front()->mutex.unlock();
					queue_.pop();
				}
			}

			void desbloq2(queue<TreeNode*>& queue_) {
				while (!queue_.empty()) {
					queue_.front()->mutex.unlock();
					queue_.pop();
				}
			}

			TreeNode* insertar(const int& key, TreeNode* ptr, queue<TreeNode*>& queue_) {
				ptr->mutex.lock();
				queue_.push(ptr);
				if (ptr->cant_llave <= 2 * d) {
					desbloq1(queue_);
				}
				if (ptr->hoja) {
					ptr->insertar_k(key);
					if (ptr->cant_llave <= 2 * d) {
						return NULL;
					}
					TreeNode* t = new TreeNode(true);
					t->mutex.lock();
					for (int i = 0; i < d + 1; i++) {
						int k = ptr->eliminar_k();
						t->insertar_k(k);
					}
					ptr->obtener(t);
					t->mutex.unlock();
					return t;
				}
				TreeNode* tmp = nullptr;
				if (key < ptr->key[0])
					tmp = insertar(key, ptr->ptr[0], queue_);
				else if (key >= ptr->key[ptr->cant_llave - 1])
					tmp = insertar(key, ptr->ptr[ptr->cant_llave], queue_);
				else
					for (int i = 0; i < ptr->cant_llave - 1; i++)
						if (key >= ptr->key[i] && key < ptr->key[i + 1]) {
							tmp = insertar(key, ptr->ptr[i + 1], queue_);
							break;
						}
				if (!tmp) {
					return tmp;
				}
				ptr->insertar_k(tmp->key[0]);
				if (!tmp->hoja) {
					tmp->eliminar_k2(tmp->obtener_primera_k());
				}
				ptr->insertar_p(tmp);
				if (ptr->cant_llave <= 2 * d)
					return NULL;
				TreeNode* x = new TreeNode(false);
				x->mutex.lock();
				for (int i = 0; i < d + 1; i++) {
					x->insertar_k(ptr->eliminar_k());
					x->insertar_p(ptr->eliminar_p());
				}
				x->mutex.unlock();
				if (ptr == raiz) {
					TreeNode* nueva_raiz = new TreeNode(false);
					nueva_raiz->mutex.lock();
					nueva_raiz->insertar_k(x->eliminar_k1());
					nueva_raiz->insertar_p(ptr);
					nueva_raiz->insertar_p(x);
					raiz = nueva_raiz;
					nueva_raiz->mutex.unlock();
				}
				return x;
			}
		};

	}  // namespace Concurrent
}	// namespace EDA

#endif  // SOURCE_B_LINK_HPP_
