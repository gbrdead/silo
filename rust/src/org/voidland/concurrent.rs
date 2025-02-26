pub mod queue;
pub mod turning_grille;

use std::env;
use std::sync::LazyLock;


pub static VERBOSE: LazyLock<bool> = LazyLock::new(||
    {
        env::var("VERBOSE").is_ok() && env::var("VERBOSE").unwrap().eq_ignore_ascii_case("true")
    });
