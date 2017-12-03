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
    rocket::ignite().mount("/", routes![index]).launch();
}
