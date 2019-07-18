#![feature(proc_macro_hygiene, decl_macro)]

#[macro_use]
extern crate rocket;

extern crate timeit;

extern crate rand;

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
    // println!("Boooo");
    rocket::ignite().mount("/", routes![index]).launch();
}
