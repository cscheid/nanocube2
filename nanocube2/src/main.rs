mod ref_counted_vec;

fn main() {
    println!("Hello, world!");
    let mut v = ref_counted_vec::RefCountedVec::<i32>::new();
    v.insert(1);
    v.insert(2);
    v.make_ref(0);
    v.make_ref(1);
    v.make_ref(0);
    v.release_ref(0);
    println!("This is our refcountedvec: {:?}", v);
    v.release_ref(0);
    println!("This is our refcountedvec: {:?}", v);

    println!("Value: {}", v.at(0));
    {
        *v.at_mut(0) = 3;
    }
    println!("Value: {}", v.at(0));
    println!("Compaction transposition map: {:?}", v.compact());
    println!("This is our refcountedvec: {:?}", v);
}
