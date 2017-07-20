extern crate rand;

mod ref_counted_vec;
mod nanocube;
mod naivecube;
mod cube;
mod tests;
mod query;

fn main() {
    // ref_counted_vec::smoke_test();
    // nanocube::smoke_test();
    // naivecube::smoke_test();
    tests::run();
}
