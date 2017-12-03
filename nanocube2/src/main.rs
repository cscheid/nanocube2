#![feature(plugin)]
#![plugin(rocket_codegen)]

extern crate rand;
extern crate rocket;

#[macro_use]
extern crate timeit;

mod ref_counted_vec;
mod nanocube;
mod naivecube;
mod cube;
mod tests;
mod query;

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

fn main() {
    // ref_counted_vec::smoke_test();
    nanocube::smoke_test();
    // naivecube::smoke_test();
    // tests::run();

    rocket::ignite().mount("/", routes![index]).launch();
}

// fn main() {
//     // ref_counted_vec::smoke_test();
//     nanocube::smoke_test();
//     // naivecube::smoke_test();
//     // tests::run();
// }
