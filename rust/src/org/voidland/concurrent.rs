pub mod queue;
pub mod turning_grille;

use std::env;
use once_cell::sync::Lazy;


pub static VERBOSE: Lazy<bool> = Lazy::new(||
    {
        env::var("VERBOSE").is_ok() && env::var("VERBOSE").unwrap().eq_ignore_ascii_case("true")
    });
