#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// ITERATOR: move the traversal out of the collection and into an object, so the
// client can walk a structure it knows nothing about.
//
// Two interfaces, and that is the whole pattern:
//
//     Iterable<T>::getIterator()  ->  Iterator<T>*
//     Iterator<T>::hasNext() / next()
//
// Each collection decides how it is walked - down a next pointer, in-order
// through a tree, forward through a vector - and the client sees one shape.
// Changing the structure does not touch the loop; adding a structure does not
// touch the client either.
//
// Ownership: the collection owns its elements, the caller owns the iterator it
// asked for, and Iterator<T>'s virtual destructor is what makes deleting it
// through the base pointer safe.

template <typename T>
class Iterator {
public:
    virtual ~Iterator() = default;

    // Progress lives in the iterator, not in the collection: two iterators over
    // the same playlist have two independent positions.
    virtual bool hasNext() const = 0;
    virtual T next() = 0;
};

template <typename T>
class Iterable {
public:
    virtual ~Iterable() = default;

    // Hands over ownership: the caller decides when the walk ends.
    virtual Iterator<T> *getIterator() = 0;
};

class LinkedListIterator;

// The node is also the collection here - getIterator() walks from that node
// onward. A production version separates the two (a container owning a chain of
// private nodes); the iterator would not change.
class LinkedList : public Iterable<int> {
public:
    explicit LinkedList(int value) : data(value) {}

    void append(int value) {
        LinkedList *node = this;
        while (node->next != nullptr) {
            node = node->next.get();
        }
        node->next = make_unique<LinkedList>(value);
    }

    Iterator<int> *getIterator() override;

    int data;

    // The head owns the rest of the chain, so destroying the head frees the
    // list, and copying a node is not a thing that makes sense.
    unique_ptr<LinkedList> next;
};

class BinaryTree : public Iterable<int> {
public:
    explicit BinaryTree(int value) : data(value) {}

    Iterator<int> *getIterator() override;

    int data;

    unique_ptr<BinaryTree> left;
    unique_ptr<BinaryTree> right;
};

class Song {
public:
    Song(string title, string artist) : title(std::move(title)), artist(std::move(artist)) {}

    string title;
    string artist;
};

ostream &operator<<(ostream &out, const Song &song) {
    return out << song.title << " by " << song.artist;
}

class Playlist : public Iterable<Song> {
public:
    void addSong(Song song) {
        songs_.push_back(std::move(song));
    }

    const vector<Song> &songs() const {
        return songs_;
    }

    Iterator<Song> *getIterator() override;

private:
    vector<Song> songs_;
};

class LinkedListIterator : public Iterator<int> {
public:
    explicit LinkedListIterator(LinkedList *head) : current_(head) {}

    bool hasNext() const override {
        return current_ != nullptr;
    }

    int next() override {
        if (current_ == nullptr) {
            throw out_of_range("linked list iterator: next() past the end");
        }
        const int value = current_->data;
        current_ = current_->next.get();
        return value;
    }

private:
    LinkedList *current_;
};

class BinaryTreeInorderIterator : public Iterator<int> {
public:
    explicit BinaryTreeInorderIterator(BinaryTree *root) {
        pushLeftSpine(root);
    }

    bool hasNext() const override {
        return !stack_.empty();
    }

    int next() override {
        if (stack_.empty()) {
            throw out_of_range("tree iterator: next() past the end");
        }
        BinaryTree *node = stack_.top();
        stack_.pop();

        const int value = node->data;
        // Everything left of the right child comes next, so push that spine.
        pushLeftSpine(node->right.get());
        return value;
    }

private:
    void pushLeftSpine(BinaryTree *node) {
        while (node != nullptr) {
            stack_.push(node);
            node = node->left.get();
        }
    }

    stack<BinaryTree *> stack_;
};

class PlaylistIterator : public Iterator<Song> {
public:
    // A reference, not a copy. Holding a vector copy would make the iterator a
    // snapshot: songs added after getIterator() would be invisible, and every
    // iterator would pay for a copy of the whole playlist.
    explicit PlaylistIterator(const vector<Song> &songs) : songs_(songs) {}

    bool hasNext() const override {
        return index_ < songs_.size();
    }

    Song next() override {
        if (!hasNext()) {
            throw out_of_range("playlist iterator: next() past the end");
        }
        return songs_[index_++];
    }

private:
    const vector<Song> &songs_;
    size_t index_ = 0;
};

Iterator<int> *LinkedList::getIterator() {
    return new LinkedListIterator(this);
}

Iterator<int> *BinaryTree::getIterator() {
    return new BinaryTreeInorderIterator(this);
}

Iterator<Song> *Playlist::getIterator() {
    return new PlaylistIterator(songs_);
}

// The payoff: one loop that knows nothing about the structure underneath it.
template <typename T>
void printAll(const string &label, Iterator<T> &iterator) {
    cout << label;
    bool first = true;
    while (iterator.hasNext()) {
        cout << (first ? " " : ", ") << iterator.next();
        first = false;
    }
    cout << "\n";
}

int main() {
    cout << "Iterator\n\n";

    LinkedList list(1);
    list.append(2);
    list.append(3);

    //        2
    //       / \      left child 1, right child 3
    //      1   3
    BinaryTree root(2);
    root.left = make_unique<BinaryTree>(1);
    root.right = make_unique<BinaryTree>(3);

    Playlist playlist;
    playlist.addSong(Song("Admirin You", "Karan Aujla"));
    playlist.addSong(Song("Husn", "Anuv Jain"));

    cout << "1. Same loop, three structures that share nothing\n";
    {
        const auto iterator = unique_ptr<Iterator<int>>(list.getIterator());
        printAll("   linked list :", *iterator);
    }
    {
        const auto iterator = unique_ptr<Iterator<int>>(root.getIterator());
        printAll("   tree inorder:", *iterator);
    }
    {
        const auto iterator = unique_ptr<Iterator<Song>>(playlist.getIterator());
        printAll("   playlist    :", *iterator);
    }

    cout << "\n2. Position lives in the iterator, so two walks overlap\n";
    const auto first = unique_ptr<Iterator<Song>>(playlist.getIterator());
    const auto second = unique_ptr<Iterator<Song>>(playlist.getIterator());
    cout << "   first  -> " << first->next() << "\n";
    cout << "   second -> " << second->next() << "\n";
    cout << "   first  -> " << first->next() << "\n";

    cout << "\n3. An iterator is a view, not a snapshot\n";
    const auto live = unique_ptr<Iterator<Song>>(playlist.getIterator());
    cout << "   first -> " << live->next() << "\n";
    playlist.addSong(Song("Tere Vaaste", "Varun Jain"));
    cout << "   added a song after getIterator(); the rest of the walk:\n";
    while (live->hasNext()) {
        cout << "     " << live->next() << "\n";
    }

    cout << "\n4. Walking past the end is an error, not undefined behaviour\n";
    const auto exhausted = unique_ptr<Iterator<int>>(list.getIterator());
    while (exhausted->hasNext()) {
        exhausted->next();
    }
    try {
        exhausted->next();
        cout << "   accepted (should not happen)\n";
    } catch (const out_of_range &error) {
        cout << "   " << error.what() << "\n";
    }

    cout << "\nThe client loop never learned which structure it was walking.\n";

    return 0;
}
