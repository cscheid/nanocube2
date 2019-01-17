#![feature(plugin)]
#![plugin(rocket_codegen)]

extern crate rand;
extern crate rocket;

#[macro_use]
extern crate timeit;

mod cube;
mod naivecube;
mod nanocube;
mod query;
mod ref_counted_vec;
mod tests;

#[get("/")]
fn index() -> &'static str {
    "Hello, world!"
}

fn main() {
    rocket::ignite().mount("/", routes![index]).launch();
}
