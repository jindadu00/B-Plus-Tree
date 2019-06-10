//
// 518021910882  杜劲达
//
#include "utility.hpp"
#include <functional>
#include <cstddef>
#include "exception.hpp"
#include <fstream>
#include <cstring>


const static int M = 1000;

const static int L = 400;

const int empty_offset = -1;

namespace sjtu {

    template<class Key, class Value, class Compare = std::less<Key> >
    class BTree {
    private:
        typedef pair<Key, Value> value_type;
        char name[10] = "nice.txt";

        struct basic_info {
            int head = 0;
            int tail = 0;
            int root = 0;
            int end = 0;
            int tree_size = 0;
        };
        basic_info basic_information;

        int file_has_already_existed;
        int file_is_open = 0;
        FILE *file = nullptr;

        // ================================= file operation ===================================== //
        void openfile() {
            file_has_already_existed = 1;
            if (file_is_open == 0) {   //考虑反复开关
                file = fopen(name, "rb+");
                if (file == nullptr) {
                    file_has_already_existed = 0;
                    file = fopen(name, "w");   // 存在则删除重创
                    fclose(file);
                    file = fopen(name, "rb+");
                }
                else {
                    fileread(&basic_information, 0, sizeof(basic_info));
                }
                file_is_open = 1;
            }
        }

        inline void closeFile() {
            if (file_is_open == 1) {
                fclose(file);
                file_is_open = 0;
            }
        }

        inline void fileread(void *place, int offset, int size) const {
            fseek(file, offset, 0);
            fread(place, size, 1, file);
        }

        inline void filewrite(void *place, int offset, int size) const {      //本文总是读写1
            fseek(file, offset, 0);
            fwrite(place, size, 1, file);
        }

        // ================================= file operation ===================================== //


        struct Node {
            friend class BTree;

            int type;        //0代表非叶，1代表叶子
            int offset;
            int parent;      //父节点偏移量
            int prev, next;     //前后节点偏移量（两个节点写起来太烦了）
            int size;
            value_type child[M + 1];

            Node(int t = 0, int o = empty_offset, int p = empty_offset, int pre = empty_offset, int n = empty_offset,
                 int s = 0) {
                type = t;
                offset = o;
                parent = p;
                prev = pre;
                next = n;
            }

            Node(const Node &node) {
                type = node.type;
                parent = node.parent;
                size = node.size;
                prev = node.prev;
                next = node.next;
                offset = node.offset;
                for (int i = 0; i < size; i++) {
                    child[i].first = node.child[i].first;       //first->key

                    child[i].second = node.child[i].second;        //second->value
                }
            }

            Node &operator=(const Node &other) {
                type = other.type;
                parent = other.parent;
                prev = other.prev;
                next = other.next;
                size = other.size;
                offset = other.offset;
                for (int i = 0; i < size; i++) {
                    child[i].first = other.child[i].first;
                    child[i].second = other.child[i].second;
                }
                return *this;
            }

            inline int findpos(const Key &key) {      //找出当前节点中关键字位置
                int i;
                for (i = 0; i < size; i++) {
                    if (child[i].first >= key)return i;     //child[i-1]<=key<=child[i]
                }
            }

            int insert(const Key &key, const Value &value) {      //调用findpos将该点插入到整个节点中
                int pos = findpos(key);

                if (pos < size && child[pos].first == key)return empty_offset;

                for (int i = size - 1; i >= pos; i--) {
                    child[i + 1].first = child[i].first;
                    child[i + 1].second = child[i].second;
                }

                child[pos].first = key;
                child[pos].second = value;
                size++;

                return pos;
            }

        };


        void splitNode(Node &node, Node &parent) {    //分裂非根节点

            Node new_node(node.type, basic_information.end, parent.offset, empty_offset, empty_offset,
                          node.size - node.size / 2);   //分裂出来的结点只好甩文件尾,中间的给右边

            parent.insert(node.child[node.size / 2].first, new_node.offset);

            basic_information.end += sizeof(Node);

            for (int i = node.size / 2; i < node.size; i++) {       //本儿
                new_node.child[i - node.size / 2].first = node.child[i].first;
                new_node.child[i - node.size / 2].second = node.child[i].second;
            }

            if (node.type == 1) {
                new_node.prev = node.offset;
                new_node.next = node.next;
                node.next = new_node.offset;   //接入单链表
                if (new_node.next != empty_offset) {
                    Node nextNode;
                    fileread(&nextNode, new_node.next, sizeof(Node));
                    nextNode.prev = new_node.offset;
                    filewrite(&nextNode, nextNode.offset, sizeof(Node));
                }
            }
            else {
                Node tmp_node;
                for (int i = node.size / 2; i < node.size; i++) {
                    fileread(&tmp_node, node.child[i].second, sizeof(Node));      //value是儿子偏移量
                    tmp_node.parent = new_node.offset;
                    filewrite(&tmp_node, tmp_node.offset, sizeof(Node));
                }
            }

            node.size /= 2;

            filewrite(&new_node, new_node.offset, sizeof(Node));
            filewrite(&node, node.offset, sizeof(Node));
            filewrite(&parent, parent.offset, sizeof(Node));
        }

        void splitroot(Node &node) {

            Node newroot(0, basic_information.end, empty_offset, empty_offset, empty_offset, 2);
            Node new_node(0, basic_information.end, newroot.offset, empty_offset, empty_offset,
                          node.size - node.size / 2);
            Node tmp_node;

            basic_information.end += sizeof(Node);
            basic_information.end += sizeof(Node);

            node.parent = newroot.offset;

            for (int i = node.size / 2; i < node.size; i++) {
                fileread(&tmp_node, node.child[i].second, sizeof(Node));
                tmp_node.parent = new_node.offset;
                filewrite(&tmp_node, tmp_node.offset, sizeof(Node));
            }

            for (int i = node.size / 2; i < node.size; i++) {
                new_node.child[i - node.size / 2].first = node.child[i].first;
                new_node.child[i - node.size / 2].second = node.child[i].second;
            }

            newroot.child[0].first = node.child[0].first;
            newroot.child[0].second = node.offset;

            newroot.child[1].first = new_node.child[0].first;
            newroot.child[1].second = new_node.offset;

            basic_information.root = newroot.offset;

            node.size /= 2;

            filewrite(&new_node, new_node.offset, sizeof(Node));
            filewrite(&node, node.offset, sizeof(Node));
            filewrite(&newroot, newroot.offset, sizeof(Node));

        }

        // Your private members go here
    public:


        class const_iterator;

        class iterator {
        private:
            // Your private members go here
        public:
            /*bool modify(const Value& value){

            }
            iterator() {
                // TODO Default Constructor
            }
            iterator(const iterator& other) {
                // TODO Copy Constructor
            }
            // Return a new iterator which points to the n-next elements
            iterator operator++(int) {
                // Todo iterator++
            }
            iterator& operator++() {
                // Todo ++iterator
            }
            iterator operator--(int) {
                // Todo iterator--
            }
            iterator& operator--() {
                // Todo --iterator
            }
            // Overloaded of operator '==' and '!='
            // Check whether the iterators are same
            bool operator==(const iterator& rhs) const {
                // Todo operator ==
            }
            bool operator==(const const_iterator& rhs) const {
                // Todo operator ==
            }
            bool operator!=(const iterator& rhs) const {
                // Todo operator !=
            }
            bool operator!=(const const_iterator& rhs) const {
                // Todo operator !=
            }*/
        };

        class const_iterator {
            // it should has similar member method as iterator.
            //  and it should be able to construct from an iterator.
        private:
            // Your private members go here
        public:
            /*
            const_iterator() {
                // TODO
            }
            const_iterator(const const_iterator& other) {
                // TODO
            }
            const_iterator(const iterator& other) {
                // TODO
            }*/
            // And other methods in iterator, please fill by yourself.
        };
        // Default Constructor and Copy Constructor


        BTree() {
            file = NULL;
            openfile();

            if (file_has_already_existed == 0) {

                Node rootNode(0, sizeof(basic_info), empty_offset, empty_offset, empty_offset, 1);
                Node firstNode(1, sizeof(basic_info) + sizeof(Node), rootNode.offset, empty_offset, empty_offset, 0);
                rootNode.child[0].second = firstNode.offset;

                basic_information.root = rootNode.offset;
                basic_information.head = basic_information.tail = firstNode.offset;
                basic_information.tree_size = 0;

                basic_information.end = sizeof(basic_info) + 2 * sizeof(Node);

                filewrite(&basic_information, 0, sizeof(basic_info));
                filewrite(&rootNode, rootNode.offset, sizeof(Node));
                filewrite(&firstNode, firstNode.offset, sizeof(Node));
            }
            // Todo Default
        }

        BTree(const BTree &other) {

            // Todo Copy
        }

        BTree &operator=(const BTree &other) {

            // Todo Assignment
        }
        // Clear the BTree

        ~BTree() {
            closeFile();
        }

        Value at(const Key &key) {

            Node tmp_node;
            fileread(&tmp_node, basic_information.root, sizeof(Node));

            int i;

            while (tmp_node.type == 0) {
                i = 0;
                while (i < tmp_node.size && key >= tmp_node.child[i].first)i++;
                if (i > 0)i--;      //比最小儿子小不代表比最小孙子小
                fileread(&tmp_node, tmp_node.child[i].second, sizeof(Node));
            }
            for (i = 0; i < tmp_node.size; i++) {   // 找到叶节点啦
                if (tmp_node.child[i].first == key)
                    break;
            }
            if (i < tmp_node.size)
                return tmp_node.child[i].second;
            else
                return empty_offset;    //配合count()
        }

        // Insert: Insert certain Key-Value into the database
        // Return a pair, the first of the pair is the iterator point to the new
        // element, the second of the pair is Success if it is successfully inserted
        pair<iterator, OperationResult> insert(const Key &key, const Value &value) {
            pair<iterator, OperationResult> p;

            p.second = Success;

            Node node;


            fileread(&node, basic_information.root, sizeof(Node));

            int i;

            while (node.type == 0) {
                i = 0;
                while (i < node.size && node.child[i].first < key)i++;
                if (i == 0) {    //沿路直接改中间节点,不同于at()
                    node.child[0].first = key;
                    filewrite(&node, node.offset, sizeof(Node));
                }
                else i--;
                fileread(&node, node.child[i].second, sizeof(Node));
            }

            basic_information.tree_size++;

            Node parent;
            fileread(&parent, node.parent, sizeof(Node));

            if (i == 0)
                parent.child[0].first = node.child[0].first;    //中间节点没有value

            if (node.size >= L) {    //要分裂了哥
                splitNode(node, parent);
                node = parent;

                if (parent.offset != basic_information.root)     //不小心->根没有parent
                    fileread(&parent, node.parent, sizeof(Node));

                while (node.size > M - 1 && node.offset != basic_information.root) {    //一路向上分裂
                    splitNode(node, parent);

                    node = parent;
                    if (node.offset != basic_information.root)
                        fileread(&parent, parent.parent, sizeof(Node));
                }
                if (node.offset == basic_information.root && node.size > M - 1) {    //分裂到根还没完
                    splitroot(node);
                }
            }
            else
                filewrite(&node, node.offset, sizeof(Node));

            //filewrite(&basic_information, 0, sizeof(basic_info));      照理说应该要但与测试无关

            return p;
            // TODO insert function
        }
        // Erase: Erase the Key-Value
        // Return Success if it is successfully erased
        // Return Fail if the key doesn't exist in the database
        /*OperationResult erase(const Key& key) {
            // TODO erase function
        }*/
        // Return a iterator to the beginning
        iterator begin() { return iterator(); }

        const_iterator cbegin() const { return const_iterator(); }

        // Return a iterator to the end(the next element after the last)
        iterator end() { return iterator(); }

        const_iterator cend() const { return const_iterator(); }

        // Check whether this BTree is empty
        bool empty() const {
            return basic_information.tree_size == 0;
        }

        // Return the number of <K,V> pairs
        size_t size() const {
            return basic_information.tree_size;
        }

        /**
         * Returns the number of elements with key
         *   that compares equivalent to the specified argument,
         * The default method of check the equivalence is !(a < b || b > a)
         */

        size_t count(const Key &key) const {
            int flag = at(key);
            if (flag == empty_offset)return 0;
            else return 1;    //没见过有重复元素的bpt
        }

        /**
         * Finds an element with key equivalent to key.
         * key value of the element to search for.
         * Iterator to an element with key equivalent to key.
         *   If no such element is found, past-the-end (see end()) iterator is
         * returned.
         */
        iterator find(const Key &key) { return iterator(); }

        const_iterator find(const Key &key) const { return const_iterator(); }
    };// namespace sjtu
}
