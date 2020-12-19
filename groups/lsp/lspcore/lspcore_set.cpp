#include <bdlb_arrayutil.h>
#include <bsl_algorithm.h>
#include <bsl_vector.h>
#include <bslma_allocator.h>
#include <bsls_assert.h>
#include <lspcore_listutil.h>
#include <lspcore_set.h>

using namespace BloombergLP;

namespace lspcore {
namespace {

// Tree Balancing
// =========
// A set is a binary tree ordered using the supplied "less than" predicate. To
// insert a new element, we traverse the tree looking for an equivalent
// element. If we find one, then we return the input tree (set is unchanged).
// If we don't find it, then we return a new tree that shares as much as
// possible with the old tree, but also contains the new element.
//
// Repeatedly inserting this way yields a linked list in the worst case (insert
// monotonically increasing values starting from the empty set). To keep the
// tree balanced, each node stores one plus the height of its larger subtree.
// Then, after an insertion, we examine the difference between the heights of a
// tree's two subtrees. If the magnitude of the difference is greater than one,
// then we "rotate" the tree to reduce the magnitude of the difference. Making
// sure to do this after every modification to the tree, we ensure that the
// tree remains as balanced as possible, thus maintaining logarithmic lookup
// time complexity.
//
// There are two kinds of rotations that the balancing operation might do:
// right and left. Balancing the tree might take zero, one, or two rotations.
//
// Right Rotation
// --------------
// Imagine grabbing the left tree by the "B" node and shaking it a little until
// the "middle" node falls off of "B" and attaches itself to "A". This is a
// tree rotation to the right.
//
//           A                 B
//         ./ \.             ./ \.
//         B  high    →    low   A
//       ./ \.                 ./ \.
//     low  middle         middle  high
//
// Left Rotation
// -------------
// A tree rotation to the left is the reverse operation. Imagine grabbing the
// left tree by the "A" node and shaking it a little until the "middle" node
// falls off of "A" and attaches itself to "B". This is a tree rotation to the
// left.
//
//         B                     A
//       ./ \.                 ./ \.
//     low   A        →        B  high
//         ./ \.             ./ \.
//     middle  high        low  middle
//
// Left-Leaning Special Case
// -------------------------
// Sometimes rotating a tree does not balance it, it just shifts the weight to
// the other side. In these cases, a subtree must first be rotated in the
// opposite direction, and then the larger tree can be rotated, resulting in a
// balanced tree.
//
//           C                 C                B
//         ./ \.             ./ \.            ./ \.
//         A          →      B         →      A   C
//       ./ \.             ./ \.
//           B             A
//
// Right-Leaning Special Case
// --------------------------
// The right-leaning special case is analagous to the left-leaning special
// case, but the long branch is to the right rather than to the left.
//
//
//           A                 A                B
//         ./ \.             ./ \.            ./ \.
//             C      →          B     →      A   C
//           ./ \.             ./ \.
//           B                     C
//

const Set* rotateRight(const Set* set, bslma::Allocator* allocator) {
    //
    //           A                 B
    //         ./ \.             ./ \.
    //         B  high    →    low   A
    //       ./ \.                 ./ \.
    //     low  middle         middle  high
    //
    const Set* A = set;
    BSLS_ASSERT_OPT(A);
    const Set* B    = A->left();
    const Set* high = A->right();
    BSLS_ASSERT_OPT(B);
    const Set* low    = B->left();
    const Set* middle = B->right();

    const Set* Aout = new (*allocator) Set(A->value(), middle, high);
    const Set* Bout = new (*allocator) Set(B->value(), low, Aout);
    return Bout;
}

const Set* rotateLeft(const Set* set, bslma::Allocator* allocator) {
    //         B                     A
    //       ./ \.                 ./ \.
    //     low   A        →        B  high
    //         ./ \.             ./ \.
    //     middle  high        low  middle
    //
    const Set* B = set;
    BSLS_ASSERT_OPT(B);
    const Set* low = B->left();
    const Set* A   = B->right();
    BSLS_ASSERT_OPT(A);
    const Set* middle = A->left();
    const Set* high   = A->right();

    const Set* Bout = new (*allocator) Set(B->value(), low, middle);
    const Set* Aout = new (*allocator) Set(A->value(), Bout, high);
    return Aout;
}

// right child height minus left child height
int heightDiff(const Set* set) {
    if (!set) {
        return 0;
    }

    return Set::height(set->right()) - Set::height(set->left());
}

const Set* balance(const Set* set, bslma::Allocator* allocator) {
    if (!set) {
        return set;
    }

    switch (const int diff = heightDiff(set)) {
        case 2:
            if (heightDiff(set->right()) == -1) {
                // right-leaning special case
                BSLS_ASSERT(set->right());

                return rotateLeft(
                    new (*allocator) Set(set->value(),
                                         set->left(),
                                         rotateRight(set->right(), allocator)),
                    allocator);
            }
            else {
                return rotateLeft(set, allocator);
            }
        case -2:
            if (heightDiff(set->left()) == 1) {
                // left-leaning special case
                BSLS_ASSERT(set->left());

                return rotateRight(new (*allocator)
                                       Set(set->value(),
                                           rotateLeft(set->left(), allocator),
                                           set->right()),
                                   allocator);
            }
            else {
                return rotateRight(set, allocator);
            }
        default:
            (void)diff;
            BSLS_ASSERT(diff == 0 || diff == 1 || diff == -1);
            return set;
    }
}

}  // namespace

Set::Set(const bdld::Datum& value, const Set* left, const Set* right)
: d_value(value)
, d_left(left)
, d_right(right) {
    setHeight(bsl::max(height(left), height(right)) + 1);
}

void Set::setHeight(int height) {
#ifdef BSLS_PLATFORM_CPU_32_BIT
    d_height = height;
#else
    // Make sure that 'height' fits in six bits.
    BSLS_ASSERT_OPT((0x3F & height) == height);

    // Bottom three bits go to the low end of 'd_right'. Next three bits go to
    // the low end of 'd_left', i.e.
    //
    //     d_left bits:     _ _ _ _ _ ... a b c
    //     d_right bits:    _ _ _ _ _ ... d e f
    //     height bits:     ... _ _ a b c d e f
    //
    d_right = asPtr(asInt(d_right) | (height & 7));
    d_left  = asPtr(asInt(d_left) | ((height >> 3) & 7));
#endif
}

const Set* Set::insert(const Set*         set,
                       const bdld::Datum& value,
                       const Comparator&  before,
                       bslma::Allocator*  allocator) {
    if (!set) {
        return new (*allocator) Set(value, 0, 0);
    }

    if (before(value, set->value())) {
        return balance(new (*allocator)
                           Set(set->value(),
                               insert(set->left(), value, before, allocator),
                               set->right()),
                       allocator);
    }

    if (before(set->value(), value)) {
        return balance(new (*allocator)
                           Set(set->value(),
                               set->left(),
                               insert(set->right(), value, before, allocator)),
                       allocator);
    }

    return set;
}

bool Set::contains(const Set*         set,
                   const bdld::Datum& value,
                   const Comparator&  before) {
    if (!set) {
        return false;
    }

    if (before(value, set->value())) {
        return contains(set->left(), value, before);
    }

    if (before(set->value(), value)) {
        return contains(set->right(), value, before);
    }

    return true;
}

bdld::Datum Set::toList(const Set*        set,
                        int               typeOffset,
                        bslma::Allocator* allocator) {
    if (!set) {
        return bdld::Datum::createNull();
    }

    const bdld::Datum elements[] = {
        set->value(),
        toList(set->left(), typeOffset, allocator),
        toList(set->right(), typeOffset, allocator)
    };

    return ListUtil::createList(
        elements, bdlb::ArrayUtil::end(elements), typeOffset, allocator);
}

}  // namespace lspcore
